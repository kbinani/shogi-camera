import AVFoundation
import ShogiCamera
import UIKit

protocol StartViewDelegate: AnyObject {
  func startViewDidStartGame(_ vc: StartView, with analyzer: Analyzer)
  func startViewPresentHelpViewController(_ v: StartView)
}

class StartView: UIView {
  weak var delegate: StartViewDelegate?

  private var startAsBlackButton: RoundButton!
  private var startAsWhiteButton: RoundButton!
  private let analyzer: Analyzer?
  private var videoView: UIView!
  private var previewLayer: AVCaptureVideoPreviewLayer?
  private var videoOverlay: VideoOverlay!
  private var messageLabel: UILabel!
  private var helpButton: RoundButton!

  enum State {
    case waitingStableBoard
    case ready
    case cameraNotAvailable
  }
  private var state: State = .cameraNotAvailable {
    didSet {
      switch state {
      case .cameraNotAvailable:
        self.messageLabel.text = "カメラを初期化できませんでした"
        self.startAsBlackButton.isEnabled = false
        self.startAsWhiteButton.isEnabled = false
      case .waitingStableBoard:
        self.messageLabel.text = "将棋盤全体が映る位置で固定してください"
        self.startAsBlackButton.isEnabled = false
        self.startAsWhiteButton.isEnabled = false
      case .ready:
        self.messageLabel.text = "準備ができました"
        self.startAsBlackButton.isEnabled = true
        self.startAsWhiteButton.isEnabled = true
      }
    }
  }

  init(analyzer reusable: Analyzer?) {
    if let reusable {
      reusable.reset()
      self.analyzer = reusable
    } else {
      self.analyzer = .init()
    }
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

    let videoOverlay = VideoOverlay()
    self.videoOverlay = videoOverlay
    videoView.layer.insertSublayer(videoOverlay, above: self.previewLayer)
    videoOverlay.contentsScale = self.traitCollection.displayScale

    let startAsBlackButton = RoundButton(type: .custom)
    startAsBlackButton.setTitle("先手番で対局開始", for: .normal)
    startAsBlackButton.isEnabled = false
    startAsBlackButton.addTarget(self, action: #selector(startAsBlackButtonDidTouchUpInside(_:)), for: .touchUpInside)
    self.addSubview(startAsBlackButton)
    self.startAsBlackButton = startAsBlackButton

    let startAsWhiteButton = RoundButton(type: .custom)
    startAsWhiteButton.colorSet = .init(
      normal: .init(title: .black, background: .white),
      disabled: RoundButton.ColorSet.default.disabled,
      highlighted: .init(title: .white, background: .black)
    )
    startAsWhiteButton.setTitle("後手番で対局開始", for: .normal)
    startAsWhiteButton.isEnabled = false
    startAsWhiteButton.addTarget(self, action: #selector(startAsWhiteButtonDidTouchUpInside(_:)), for: .touchUpInside)
    self.addSubview(startAsWhiteButton)
    self.startAsWhiteButton = startAsWhiteButton

    let messageLabel = UILabel()
    messageLabel.lineBreakMode = .byWordWrapping
    messageLabel.font = messageLabel.font.withSize(messageLabel.font.pointSize * 1.5)
    messageLabel.numberOfLines = 0
    messageLabel.textAlignment = .center
    messageLabel.minimumScaleFactor = 0.5
    self.addSubview(messageLabel)
    self.messageLabel = messageLabel

    let helpButton = RoundButton(type: .custom)
    helpButton.setTitle("ヘルプ", for: .normal)
    helpButton.addTarget(self, action: #selector(helpButtonDidTouchUpInside(_:)), for: .touchUpInside)
    self.addSubview(helpButton)
    self.helpButton = helpButton

    self.state = .cameraNotAvailable
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
      videoOverlay.frame = .init(x: x, y: y, width: videoDimension.width * scale, height: videoDimension.height * scale)
    } else {
      videoOverlay.frame = .init(origin: .zero, size: video.size)
    }

    var buttons = bounds.removeFromTop(90)
    let buttonWidth = (buttons.width - margin) / 2
    self.startAsBlackButton.frame = buttons.removeFromLeft(buttonWidth)
    self.startAsWhiteButton.frame = buttons.removeFromRight(buttonWidth)
    bounds.removeFromTop(margin)

    var help = bounds.removeFromBottom(44)
    let helpWidth = helpButton.intrinsicContentSize.width + 2 * margin
    help.removeFromLeft((help.width - helpWidth) / 2)
    self.helpButton.frame = help.removeFromLeft(helpWidth)

    bounds.removeFromBottom(margin)

    self.messageLabel.frame = bounds
  }

  @objc private func startAsBlackButtonDidTouchUpInside(_ sender: UIButton) {
    guard let analyzer else {
      return
    }
    if let connection = previewLayer?.connection {
      analyzer.captureSession.removeConnection(connection)
    }
    //    analyzer.startGame(userColor: .Black, aiLevel: 0)
    analyzer.startGame(userColor: .Black, csaServer: "192.168.1.33", csaPort: 4081, username: "ShogiCamera", password: "foo")
    delegate?.startViewDidStartGame(self, with: analyzer)
  }

  @objc private func startAsWhiteButtonDidTouchUpInside(_ sender: UIButton) {
    guard let analyzer else {
      return
    }
    if let connection = previewLayer?.connection {
      analyzer.captureSession.removeConnection(connection)
    }
    analyzer.startGame(userColor: .White, aiLevel: 0)
    delegate?.startViewDidStartGame(self, with: analyzer)
  }

  @objc private func helpButtonDidTouchUpInside(_ sender: UIButton) {
    delegate?.startViewPresentHelpViewController(self)
  }
}

extension StartView: AnalyzerDelegate {
  func analyzerDidUpdateStatus(_ analyzer: Analyzer) {
    let status = analyzer.status

    self.videoOverlay.status = status
    switch state {
    case .waitingStableBoard:
      if status.boardReady == true {
        state = .ready
      }
    case .ready:
      if status.boardReady != true {
        state = .waitingStableBoard
      }
    case .cameraNotAvailable:
      state = .waitingStableBoard
    }
  }
}
