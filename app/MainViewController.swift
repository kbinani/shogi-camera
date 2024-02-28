import UIKit

class MainViewController: UIViewController {
  private var startAsBlackButton: UIButton!
  private var startAsWhiteButton: UIButton!

  override func viewDidLoad() {
    super.viewDidLoad()

    self.view.backgroundColor = .darkGray

    let startAsBlackButton = styleButton(UIButton(type: .custom))
    startAsBlackButton.setTitle("先手でスタート", for: .normal)
    startAsBlackButton.setTitleColor(.white, for: .normal)
    startAsBlackButton.setTitleColor(.black, for: .highlighted)
    startAsBlackButton.backgroundColor = .black
    startAsBlackButton.tintColor = .white
    startAsBlackButton.addTarget(
      self, action: #selector(startAsBlackButtonDidTouchUpInside(_:)), for: .touchUpInside)
    startAsBlackButton.addTarget(
      self, action: #selector(startAsBlackButtonDidTouchDown(_:)), for: .touchDown)
    self.view.addSubview(startAsBlackButton)
    self.startAsBlackButton = startAsBlackButton

    let startAsWhiteButton = styleButton(UIButton(type: .custom))
    startAsWhiteButton.setTitle("後手でスタート", for: .normal)
    startAsWhiteButton.setTitleColor(.black, for: .normal)
    startAsWhiteButton.setTitleColor(.white, for: .highlighted)
    startAsWhiteButton.backgroundColor = .white
    startAsWhiteButton.addTarget(
      self, action: #selector(startAsWhiteButtonDidTouchUpInside(_:)), for: .touchUpInside)
    startAsWhiteButton.addTarget(
      self, action: #selector(startAsWhiteButtonDidTouchDown(_:)), for: .touchDown)
    self.view.addSubview(startAsWhiteButton)
    self.startAsWhiteButton = startAsWhiteButton
  }

  private func styleButton(_ button: UIButton) -> UIButton {
    button.titleLabel?.textAlignment = .center
    button.layer.cornerRadius = 15
    return button
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    let size = self.view.bounds.size
    let margin: CGFloat = 15
    let buttonWidth = (size.width - 3 * margin) / 2
    let buttonHeight: CGFloat = 90
    self.startAsBlackButton.frame = .init(
      x: margin, y: size.height * 0.5, width: buttonWidth, height: buttonHeight)
    self.startAsWhiteButton.frame = .init(
      x: size.width * 0.5 + margin * 0.5, y: size.height * 0.5, width: buttonWidth,
      height: buttonHeight)
  }

  @objc private func startAsBlackButtonDidTouchUpInside(_ sender: UIButton) {
    self.startAsBlackButton.backgroundColor = .black
  }

  @objc private func startAsBlackButtonDidTouchDown(_ sender: UIButton) {
    self.startAsBlackButton.backgroundColor = .gray
  }

  @objc private func startAsWhiteButtonDidTouchUpInside(_ sender: UIButton) {
    self.startAsWhiteButton.backgroundColor = .white
  }

  @objc private func startAsWhiteButtonDidTouchDown(_ sender: UIButton) {
    self.startAsWhiteButton.backgroundColor = .gray
  }
}
