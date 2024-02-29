import AVFoundation
import ShogiCamera
import UIKit

class MainViewController: UIViewController {
  private var startAsBlackButton: UIButton!
  private var startAsWhiteButton: UIButton!
  private let analyzer: Analyzer?
  private var videoView: UIView!
  private var previewLayer: AVCaptureVideoPreviewLayer?
  private var videoOverlay: VideoOverlay!

  override init(nibName: String?, bundle: Bundle?) {
    self.analyzer = .init()
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

    if let session = analyzer?.captureSession {
      analyzer?.delegate = self
      DispatchQueue.global().async { [weak session] in
        session?.startRunning()
      }
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
    self.startAsBlackButton.backgroundColor = .black
    analyzer?.startGame(userColor: .Black, aiLevel: 0)
  }

  @objc private func startAsBlackButtonDidTouchDown(_ sender: UIButton) {
    self.startAsBlackButton.backgroundColor = .gray
  }

  @objc private func startAsWhiteButtonDidTouchUpInside(_ sender: UIButton) {
    self.startAsWhiteButton.backgroundColor = .white
    analyzer?.startGame(userColor: .White, aiLevel: 0)
  }

  @objc private func startAsWhiteButtonDidTouchDown(_ sender: UIButton) {
    self.startAsWhiteButton.backgroundColor = .gray
  }
}

extension MainViewController: AnalyzerDelegate {
  func analyzerDidChangeBoardReadieness(_ analyzer: Analyzer) {
    if analyzer.status.boardReady {
      self.startAsBlackButton.backgroundColor = .black
      self.startAsWhiteButton.backgroundColor = .white
      self.startAsBlackButton.isEnabled = true
      self.startAsWhiteButton.isEnabled = true
    } else {
      self.startAsBlackButton.isEnabled = true
      self.startAsWhiteButton.isEnabled = true
      self.startAsBlackButton.backgroundColor = .gray
      self.startAsWhiteButton.backgroundColor = .gray
    }
  }

  func analyzerDidUpdateStatus(_ analyzer: Analyzer) {
    self.videoOverlay.status = analyzer.status
  }
}
