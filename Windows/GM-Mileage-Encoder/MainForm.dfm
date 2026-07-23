object Form1: TForm1
  Left = 0
  Top = 0
  BorderStyle = bsToolWindow
  Caption = 'GM / Hummer H3 BCM Mileage Encoder'
  ClientHeight = 285
  ClientWidth = 652
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -20
  Font.Name = 'Segoe UI'
  Font.Style = []
  Position = poScreenCenter
  TextHeight = 28
  object LabelTitle: TLabel
    Left = 24
    Top = 16
    Width = 304
    Height = 21
    Caption = 'GM / Hummer H3 BCM Mileage Encoder'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -16
    Font.Name = 'Segoe UI'
    Font.Style = [fsBold]
    ParentFont = False
  end
  object LabelInput: TLabel
    Left = 24
    Top = 68
    Width = 133
    Height = 28
    Caption = 'Target Mileage:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -20
    Font.Name = 'Segoe UI'
    Font.Style = []
    ParentFont = False
  end
  object lblResult: TLabel
    Left = 24
    Top = 140
    Width = 617
    Height = 125
    AutoSize = False
    Caption = 'Output will appear here...'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -20
    Font.Name = 'Consolas'
    Font.Style = []
    ParentFont = False
    WordWrap = True
  end
  object EditMileage: TEdit
    Left = 167
    Top = 62
    Width = 354
    Height = 45
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -27
    Font.Name = 'Segoe UI'
    Font.Style = []
    ParentFont = False
    TabOrder = 0
    Text = '100000'
    OnKeyPress = EditMileageKeyPress
  end
  object ButtonCalculate: TButton
    Left = 527
    Top = 60
    Width = 100
    Height = 47
    Caption = 'Calculate'
    TabOrder = 1
    OnClick = ButtonCalculateClick
  end
end
