import Network
import ShogiCamera
import UIKit

enum WifiConnectivity: Equatable {
  case available(address: String)
  case unavailable
}

protocol MainViewPage: AnyObject {
  func mainViewPageDidDetectWifiConnectivity(_ connectivity: WifiConnectivity?)
}

class MainViewController: UIViewController {
  private var current: (UIView & MainViewPage)?
  private var monitor: NWPathMonitor? = NWPathMonitor(requiredInterfaceType: .wifi)

  override func viewDidLoad() {
    super.viewDidLoad()
    let startView = StartView(analyzer: nil, server: nil)
    startView.delegate = self
    self.current = startView
    self.view.addSubview(startView)
    monitor?.pathUpdateHandler = { [weak self] path in
      self?.handlePathUpdate(path)
    }
    monitor?.start(queue: .main)
  }

  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    monitor?.cancel()
    monitor = nil
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    self.current?.frame = .init(origin: .zero, size: self.view.bounds.size)
  }

  private func handlePathUpdate(_ path: NWPath) {
    if path.status == .satisfied, let name = path.availableInterfaces.first?.name, let address = Self.ipAddressString(name) {
      current?.mainViewPageDidDetectWifiConnectivity(.available(address: address))
    } else {
      current?.mainViewPageDidDetectWifiConnectivity(.unavailable)
    }
  }

  private static func ipAddressString(_ interfaceName: String) -> String? {
    var address: String? = nil
    var ifaddr: UnsafeMutablePointer<ifaddrs>? = nil
    guard getifaddrs(&ifaddr) == 0 else {
      return nil
    }
    var ptr = ifaddr
    while let p = ptr {
      let flags = Int32(p.pointee.ifa_flags)
      let addr = p.pointee.ifa_addr.pointee

      if (flags & (IFF_UP | IFF_RUNNING | IFF_LOOPBACK)) == (IFF_UP | IFF_RUNNING) {
        if addr.sa_family == UInt8(AF_INET) && String(cString: p.pointee.ifa_name) == interfaceName {
          var hostname = [CChar](repeating: 0, count: Int(NI_MAXHOST))
          if getnameinfo(
            p.pointee.ifa_addr,
            socklen_t(p.pointee.ifa_addr.pointee.sa_len),
            &hostname,
            socklen_t(hostname.count),
            nil,
            socklen_t(0),
            NI_NUMERICHOST) == 0
          {
            address = String(cString: hostname)
            break
          }
        }
      }
      ptr = p.pointee.ifa_next
    }
    freeifaddrs(ifaddr)
    return address
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
