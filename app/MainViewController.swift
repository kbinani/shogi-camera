import AVFoundation
import UIKit

class MainViewController: UIViewController {
  private var startAsBlackButton: UIButton!
  private var startAsWhiteButton: UIButton!
  private let session: AVCaptureSession?
  private var videoView: UIView!
  private var previewLayer: AVCaptureVideoPreviewLayer?

  override init(nibName: String?, bundle: Bundle?) {
    self.session = Self.CreateCaptureSession()
    super.init(nibName: nibName, bundle: bundle)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    self.view.backgroundColor = .darkGray
    print(self.view.window?.safeAreaInsets)

    let videoView = UIView(frame: .zero)
    self.view.addSubview(videoView)
    self.videoView = videoView

    if let session {
      let preview = AVCaptureVideoPreviewLayer(session: session)
      preview.videoGravity = .resizeAspect
      if #available(iOS 17, *) {
        preview.connection?.videoRotationAngle = 90
      } else {
        preview.connection?.videoOrientation = .portrait
      }
      videoView.layer.addSublayer(preview)
      self.previewLayer = preview
    } else {

    }

    let startAsBlackButton = styleButton(UIButton(type: .custom))
    startAsBlackButton.setTitle("先手でスタート", for: .normal)
    startAsBlackButton.setTitleColor(.white, for: .normal)
    startAsBlackButton.setTitleColor(.black, for: .highlighted)
    startAsBlackButton.backgroundColor = .gray
    startAsBlackButton.tintColor = .white
    startAsBlackButton.isEnabled = false
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
    startAsWhiteButton.backgroundColor = .gray
    startAsWhiteButton.isEnabled = false
    startAsWhiteButton.addTarget(
      self, action: #selector(startAsWhiteButtonDidTouchUpInside(_:)), for: .touchUpInside)
    startAsWhiteButton.addTarget(
      self, action: #selector(startAsWhiteButtonDidTouchDown(_:)), for: .touchDown)
    self.view.addSubview(startAsWhiteButton)
    self.startAsWhiteButton = startAsWhiteButton

    DispatchQueue.global().async { [weak session] in
      session?.startRunning()
    }
  }

  private func styleButton(_ button: UIButton) -> UIButton {
    button.titleLabel?.textAlignment = .center
    button.layer.cornerRadius = 15
    button.setTitleColor(.lightGray, for: .disabled)
    return button
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    let size = self.view.bounds.size
    let margin: CGFloat = 15

    var bounds = CGRect(origin: .zero, size: size)
    bounds.reduce(self.view.safeAreaInsets)
    bounds.expand(-margin, -margin)

    var video: CGRect = bounds.removeFromTop(size.height / 2)
    video.removeFromBottom(margin)
    self.videoView.frame = video
    self.previewLayer?.frame = .init(origin: .zero, size: video.size)

    var buttons = bounds.removeFromTop(90)
    let buttonWidth = (buttons.width - margin) / 2
    self.startAsBlackButton.frame = buttons.removeFromLeft(buttonWidth)
    self.startAsWhiteButton.frame = buttons.removeFromRight(buttonWidth)
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

  private static func CreateCaptureSession() -> AVCaptureSession? {
    guard
      let device = AVCaptureDevice.devices().first(where: {
        $0.position == AVCaptureDevice.Position.back
      })
    else {
      return nil
    }
    guard let deviceInput = try? AVCaptureDeviceInput(device: device) else {
      return nil
    }
    let videoDataOutput = AVCaptureVideoDataOutput()
    videoDataOutput.videoSettings = [
      kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_32BGRA
    ]
    //    videoDataOutput.setSampleBufferDelegate(self, queue: DispatchQueue.global())
    videoDataOutput.alwaysDiscardsLateVideoFrames = true
    let session = AVCaptureSession()
    guard session.canAddInput(deviceInput) else {
      return nil
    }
    session.addInput(deviceInput)
    guard session.canAddOutput(videoDataOutput) else {
      return nil
    }
    session.addOutput(videoDataOutput)

    session.sessionPreset = AVCaptureSession.Preset.medium
    if let range = device.activeFormat.videoSupportedFrameRateRanges.max(by: { left, right in
      left.maxFrameRate < right.maxFrameRate
    }) {
      do {
        try device.lockForConfiguration()
        defer {
          device.unlockForConfiguration()
        }
        let rate = min(max(5, range.minFrameRate), range.maxFrameRate)
        device.activeVideoMinFrameDuration = CMTime(
          seconds: 1 / rate, preferredTimescale: 1_000_000)
      } catch {
        print(error)
      }
    }
    return session
  }
}
