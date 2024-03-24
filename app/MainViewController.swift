import ShogiCamera
import UIKit

class MainViewController: UIViewController {
  private var current: UIView?

  override func viewDidLoad() {
    super.viewDidLoad()
    let startView = StartView(analyzer: nil, server: nil)
    startView.delegate = self
    self.current = startView
    self.view.addSubview(startView)
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    self.current?.frame = .init(origin: .zero, size: self.view.bounds.size)
  }
}

extension MainViewController: StartViewDelegate {
  func startViewDidStartGame(_ view: StartView, with analyzer: Analyzer, server: sci.CsaServerWrapper?) {
    let gameView = GameView(analyzer: analyzer, server: server)
    gameView.frame = .init(origin: .zero, size: self.view.bounds.size)
    gameView.delegate = self
    self.current?.removeFromSuperview()
    self.view.addSubview(gameView)
    self.current = gameView
  }

  func startViewPresentHelpViewController(_ v: StartView) {
    let vc = HelpViewController()
    vc.modalPresentationStyle = .fullScreen
    vc.delegate = self
    self.present(vc, animated: true)
  }

  func startViewPresentViewController(_ vc: UIViewController) {
    self.present(vc, animated: true)
  }
}

extension MainViewController: GameViewDelegate {
  func gameView(_ sender: GameView, presentViewController controller: UIViewController) {
    self.present(controller, animated: true)
  }

  func gameViewDidAbort(_ sender: GameView, server: sci.CsaServerWrapper?) {
    let startView = StartView(analyzer: sender.analyzer, server: server)
    startView.delegate = self
    self.current?.removeFromSuperview()
    self.current = startView
    self.view.addSubview(startView)
    self.current = startView
  }
}

extension MainViewController: HelpViewControllerDelegate {
  func helpViewControllerBack(_ v: HelpViewController) {
    v.dismiss(animated: true)
  }
}
