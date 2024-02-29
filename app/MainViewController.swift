import AVFoundation
import UIKit

class MainViewController: UIViewController {
  private var startAsBlackButton: UIButton!
  private var startAsWhiteButton: UIButton!
  private let session: AVCaptureSession?
  private var videoView: UIView!
  private var previewLayer: AVCaptureVideoPreviewLayer?
  private var videoOverlay: VideoOverlay!
  private let videoDimension: CGSize?

  override init(nibName: String?, bundle: Bundle?) {
    if let (session, dimension) = Self.CreateCaptureSession() {
      self.session = session
      self.videoDimension = dimension
    } else {
      self.session = nil
      self.videoDimension = nil
    }
    super.init(nibName: nibName, bundle: bundle)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    self.view.backgroundColor = .darkGray

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
    }

    let videoOverlay = VideoOverlay(
      status: session == nil ? .cameraNotAvailable : .waitingStableBoard)
    self.videoOverlay = videoOverlay
    videoView.layer.insertSublayer(videoOverlay, above: self.previewLayer)
    videoOverlay.contentsScale = self.traitCollection.displayScale

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

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    self.videoOverlay.contentsScale = self.traitCollection.displayScale
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
    if let videoDimension {
      let scale = min(videoDimension.width / video.width, videoDimension.height / video.height)
      let x = video.width / 2 - video.width * scale / 2
      let y = video.height / 2 - video.height * scale / 2
      videoOverlay.frame = .init(
        x: x, y: y, width: video.width * scale, height: video.height * scale)
    } else {
      videoOverlay.frame = .init(origin: .zero, size: video.size)
    }

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

  private static func CreateCaptureSession() -> (session: AVCaptureSession, dimension: CGSize)? {
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
        let dimension = device.activeFormat.formatDescription.dimensions
        return (session, CGSize(width: CGFloat(dimension.width), height: CGFloat(dimension.height)))
      } catch {
        print(error)
      }
    }
    return nil
  }
}

class VideoOverlay: CALayer {
  enum Status {
    case waitingStableBoard
    case ready
    case cameraNotAvailable
  }

  private var textLayer: CATextLayer!

  init(status: Status) {
    self.status = status
    super.init()
    let textLayer = CATextLayer()
    textLayer.isWrapped = true
    textLayer.alignmentMode = .center
    textLayer.allowsFontSubpixelQuantization = true
    self.textLayer = textLayer
    self.addSublayer(textLayer)
    self.update()
  }

  override init(layer: Any) {
    guard let layer = layer as? Self else {
      fatalError()
    }
    self.status = layer.status
    self.textLayer = layer.textLayer
    super.init(layer: layer)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSublayers() {
    super.layoutSublayers()
    var bounds = CGRect(origin: .zero, size: self.bounds.size)
    bounds.expand(-15, -15)
    self.textLayer.frame = bounds
  }

  var status: Status {
    didSet {
      guard self.status != oldValue else {
        return
      }
      self.update()
    }
  }

  override var contentsScale: CGFloat {
    didSet {
      self.textLayer.contentsScale = self.contentsScale
    }
  }

  private func update() {
    switch status {
    case .cameraNotAvailable:
      self.textLayer.string = "カメラを初期化できませんでした"
    case .waitingStableBoard:
      self.textLayer.string = "将棋盤全体が映る位置でカメラを固定してください"
    case .ready:
      break
    }
  }
}
