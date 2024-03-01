import AVFoundation
import ShogiCamera
import UIKit

protocol StartViewDelegate: AnyObject {
  func startViewDidStartGame(_ vc: StartView, with analyzer: Analyzer)
}

class StartView: UIView {
  weak var delegate: StartViewDelegate?

  private var startAsBlackButton: RoundButton!
  private var startAsWhiteButton: RoundButton!
  private let analyzer: Analyzer?
  private var videoView: UIView!
  private var previewLayer: AVCaptureVideoPreviewLayer?
  private var videoOverlay: VideoOverlay!

  init() {
    self.analyzer = .init()
    super.init(frame: .zero)

    self.backgroundColor = .darkGray

    let videoView = UIView(frame: .zero)
    self.addSubview(videoView)
    self.videoView = videoView

    if let analyzer {
      let preview = AVCaptureVideoPreviewLayer(session: analyzer.captureSession)
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
      state: analyzer == nil ? .cameraNotAvailable : .waitingStableBoard)
    self.videoOverlay = videoOverlay
    videoView.layer.insertSublayer(videoOverlay, above: self.previewLayer)
    videoOverlay.contentsScale = self.traitCollection.displayScale

    let startAsBlackButton = RoundButton(type: .custom)
    startAsBlackButton.setTitle("先手でスタート", for: .normal)
    startAsBlackButton.isEnabled = false
    startAsBlackButton.addTarget(
      self, action: #selector(startAsBlackButtonDidTouchUpInside(_:)), for: .touchUpInside)
    self.addSubview(startAsBlackButton)
    self.startAsBlackButton = startAsBlackButton

    let startAsWhiteButton = RoundButton(type: .custom)
    startAsWhiteButton.colorSet = .init(
      normal: .init(title: .black, background: .white),
      disabled: RoundButton.ColorSet.default.disabled,
      highlighted: .init(title: .white, background: .black)
    )
    startAsWhiteButton.setTitle("後手でスタート", for: .normal)
    startAsWhiteButton.isEnabled = false
    startAsWhiteButton.addTarget(
      self, action: #selector(startAsWhiteButtonDidTouchUpInside(_:)), for: .touchUpInside)
    self.addSubview(startAsWhiteButton)
    self.startAsWhiteButton = startAsWhiteButton

    if let session = analyzer?.captureSession {
      analyzer?.delegate = self
      DispatchQueue.global().async { [weak session] in
        session?.startRunning()
      }
    }
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    self.videoOverlay.contentsScale = self.traitCollection.displayScale
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    let size = self.bounds.size
    let margin: CGFloat = 15

    var bounds = CGRect(origin: .zero, size: size)
    bounds.reduce(self.safeAreaInsets)
    bounds.expand(-margin, -margin)

    var video: CGRect = bounds.removeFromTop(size.height / 2)
    video.removeFromBottom(margin)
    self.videoView.frame = video
    self.previewLayer?.frame = .init(origin: .zero, size: video.size)
    if let videoDimension = analyzer?.dimension {
      let scale = min(video.width / videoDimension.width, video.height / videoDimension.height)
      let x = video.width / 2 - videoDimension.width * scale / 2
      let y = video.height / 2 - videoDimension.height * scale / 2
      videoOverlay.frame = .init(
        x: x, y: y, width: videoDimension.width * scale, height: videoDimension.height * scale)
    } else {
      videoOverlay.frame = .init(origin: .zero, size: video.size)
    }

    var buttons = bounds.removeFromTop(90)
    let buttonWidth = (buttons.width - margin) / 2
    self.startAsBlackButton.frame = buttons.removeFromLeft(buttonWidth)
    self.startAsWhiteButton.frame = buttons.removeFromRight(buttonWidth)
  }

  @objc private func startAsBlackButtonDidTouchUpInside(_ sender: UIButton) {
    guard let analyzer else {
      return
    }
    analyzer.startGame(userColor: .Black, aiLevel: 0)
    delegate?.startViewDidStartGame(self, with: analyzer)
  }

  @objc private func startAsWhiteButtonDidTouchUpInside(_ sender: UIButton) {
    guard let analyzer else {
      return
    }
    analyzer.startGame(userColor: .White, aiLevel: 0)
    delegate?.startViewDidStartGame(self, with: analyzer)
  }
}

extension StartView: AnalyzerDelegate {
  func analyzerDidChangeBoardReadieness(_ analyzer: Analyzer) {
    let enable = analyzer.status.boardReady
    self.startAsBlackButton.isEnabled = enable
    self.startAsWhiteButton.isEnabled = enable
  }

  func analyzerDidUpdateStatus(_ analyzer: Analyzer) {
    self.videoOverlay.status = analyzer.status
  }
}
