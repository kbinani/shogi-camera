import ShogiCamera
import UIKit

@main
class AppDelegate: UIResponder, UIApplicationDelegate {
  var window: UIWindow?

  func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
    #if SHOGI_CAMERA_DEBUG
      Task.detached {
        sci.RunTests()
      }
    #endif

    let window = UIWindow(frame: UIScreen.main.bounds)
    self.window = window
    window.rootViewController = MainViewController()
    window.makeKeyAndVisible()
    return true
  }
}
