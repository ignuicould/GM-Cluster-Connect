unit MainForm;

interface

uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.StdCtrls;

type
  TForm1 = class(TForm)
    LabelTitle: TLabel;
    LabelInput: TLabel;
    EditMileage: TEdit;
    ButtonCalculate: TButton;
    lblResult: TLabel;
    procedure ButtonCalculateClick(Sender: TObject);
    procedure EditMileageKeyPress(Sender: TObject; var Key: Char);
  private
    function CleanInput(const InputStr: string): string;
    procedure CalculateMileageHex(Miles: Double);
  public
    { Public declarations }
  end;

var
  Form1: TForm1;

implementation

{$R *.dfm}

function TForm1.CleanInput(const InputStr: string): string;
var
  C: Char;
  DecimalFound: Boolean;
begin
  Result := '';
  DecimalFound := False;

  for C in InputStr do
  begin
    if CharInSet(C, ['0'..'9']) then
    begin
      Result := Result + C;
    end
    else if (C = '.') and not DecimalFound then
    begin
      Result := Result + C;
      DecimalFound := True;
    end;
  end;
end;

procedure TForm1.EditMileageKeyPress(Sender: TObject; var Key: Char);
begin
  if Key = #13 then
  begin
    Key := #0; // Suppress the annoying Windows beep sound
    ButtonCalculate.Click();
  end;
end;

procedure TForm1.CalculateMileageHex(Miles: Double);
var
  RawPulses: Double;
  Pulses: UInt64;
  MaxPulses: UInt64;
  Byte1, Byte2, Byte3, Byte4: Byte;
  Sb: TStringBuilder;
  I: Integer;
begin
  // 1. Calculate BCM VSS (Vehicle Speed Sensor) pulses (4000 pulses per mile)
  RawPulses := Miles * 4000.0;

  // Round to the nearest whole integer pulse
  Pulses := Round(RawPulses);

  // 2. Perform 32-bit bounds checking
  MaxPulses := $FFFFFFFF;
  if Pulses > MaxPulses then
  begin
    lblResult.Caption := Format('Error: Mileage exceeds maximum supported 32-bit limit (%.2f miles).', [MaxPulses / 4000.0]);
    Exit;
  end;

  // 3. Extract the 4 big-endian bytes
  Byte1 := (Pulses shr 24) and $FF;
  Byte2 := (Pulses shr 16) and $FF;
  Byte3 := (Pulses shr 8) and $FF;
  Byte4 := Pulses and $FF;

  // 4. Build the output string matching the C++ console layout
  Sb := TStringBuilder.Create();
  try
    Sb.AppendLine(Format('Target Odometer : %.0f miles', [Miles]));
    Sb.AppendLine(Format('Calculated VSS  : %u pulses (0x%s)', [Pulses, IntToHex(Pulses, 8)]));
    Sb.AppendLine(Format('Encoded Hex     : %.2x %.2x %.2x %.2x', [Byte1, Byte2, Byte3, Byte4]));

    Sb.Append('BCM 0x0080 Write: ');
    for I := 0 to 2 do
    begin
      Sb.Append(Format('%.2x %.2x %.2x %.2x', [Byte1, Byte2, Byte3, Byte4]));
      if I < 2 then
        Sb.Append(' ');
    end;

    lblResult.Caption := Sb.ToString();
  finally
    Sb.Free();
  end;
end;

procedure TForm1.ButtonCalculateClick(Sender: TObject);
var
  CleanedInput: string;
  Miles: Double;
  Code: Integer;
begin
  CleanedInput := CleanInput(EditMileage.Text);

  if CleanedInput.IsEmpty then
  begin
    lblResult.Caption := 'Error: Invalid mileage input. Please enter a valid number.';
    Exit;
  end;

  Val(CleanedInput, Miles, Code);
  if Code <> 0 then
  begin
    lblResult.Caption := 'Error parsing input: Invalid numeric format.';
    Exit;
  end;

  if Miles < 0 then
  begin
    lblResult.Caption := 'Error: Mileage cannot be negative.';
    Exit;
  end;

  CalculateMileageHex(Miles);
end;

end.
