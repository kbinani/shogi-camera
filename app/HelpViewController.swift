import UIKit

protocol HelpViewControllerDelegate: AnyObject {
  func helpViewControllerBack(_ vc: HelpViewController)
}

class HelpViewController: UIViewController {
  weak var delegate: HelpViewControllerDelegate?

  private var backButton: RoundButton!
  private var titleLabel: UILabel!

  override func viewDidLoad() {
    super.viewDidLoad()

    let backButton = RoundButton(type: .custom)
    backButton.setTitle("戻る", for: .normal)
    backButton.addTarget(
      self, action: #selector(backButtonDidTouchUpInside(_:)), for: .touchUpInside)
    self.view.addSubview(backButton)
    self.backButton = backButton

    let titleLabel = UILabel()
    titleLabel.text = "使い方"
    titleLabel.font = UIFont.boldSystemFont(ofSize: titleLabel.font.pointSize)
    titleLabel.textAlignment = .center
    self.view.addSubview(titleLabel)
    self.titleLabel = titleLabel
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    self.view.backgroundColor = .darkGray
    let margin: CGFloat = 15
    var bounds = CGRect(origin: .zero, size: self.view.bounds.size)
    bounds.reduce(self.view.safeAreaInsets)
    bounds.expand(-margin, -margin)
    var backButton = bounds.removeFromTop(44)
    self.backButton.frame = backButton.removeFromLeft(
      self.backButton.intrinsicContentSize.width + 2 * margin)
    bounds.removeFromTop(margin)
    self.titleLabel.frame = bounds.removeFromTop(self.titleLabel.intrinsicContentSize.height)
    bounds.removeFromTop(margin)
  }

  @objc private func backButtonDidTouchUpInside(_ sender: UIButton) {
    self.delegate?.helpViewControllerBack(self)
  }
}
