import UIKit

class RoundButton: UIButton {
  struct Color {
    let title: UIColor
    let background: UIColor
  }
  struct ColorSet {
    let normal: Color
    let disabled: Color
    let highlighted: Color

    static let `default`: ColorSet = .init(
      normal: .init(title: .white, background: .black),
      disabled: .init(title: .lightGray, background: .gray),
      highlighted: .init(title: .black, background: .white)
    )
  }

  var colorSet: ColorSet = .default {
    didSet {
      setTitleColor(colorSet.normal.title, for: .normal)
      setTitleColor(colorSet.disabled.title, for: .disabled)
      setTitleColor(colorSet.highlighted.title, for: .highlighted)
      update()
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    self.layer.cornerRadius = 15
    self.titleLabel?.textAlignment = .center
    setTitleColor(colorSet.normal.title, for: .normal)
    setTitleColor(colorSet.disabled.title, for: .disabled)
    setTitleColor(colorSet.highlighted.title, for: .highlighted)
    backgroundColor = colorSet.normal.background
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override var isEnabled: Bool {
    didSet {
      update()
    }
  }

  override var isHighlighted: Bool {
    didSet {
      update()
    }
  }

  override var isSelected: Bool {
    didSet {
      update()
    }
  }

  private func update() {
    let color: Color
    if isEnabled {
      if isHighlighted {
        color = colorSet.highlighted
      } else {
        color = colorSet.normal
      }
    } else {
      color = colorSet.disabled
    }
    backgroundColor = color.background
  }
}
