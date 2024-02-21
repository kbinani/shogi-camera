import AVFoundation
import ShogiCameraInput

class Reader {
  private let engine = AVAudioEngine()
  private let node = AVAudioPlayerNode()
  private var file: AVAudioFile!

  init?() {
    guard let voice = Bundle.main.url(forResource: "voice", withExtension: "wav") else {
      return nil
    }
    do {
      file = try AVAudioFile(forReading: voice)
      let session = AVAudioSession.sharedInstance()
      try session.setCategory(.playAndRecord)
      try session.setActive(true)
      engine.attach(node)
      engine.connect(node, to: engine.mainMixerNode, format: file.processingFormat)
      node.volume = 24
      try engine.start()
      node.play()
    } catch {
      print(error)
      return nil
    }
  }

  deinit {
    node.stop()
    engine.stop()
  }

  struct Position {
    let square: (file: Int, rank: Int)
    let time: TimeInterval
  }
  private let positions: [Position] = [
    .init(square: (8, 0), time: 1.648),
    .init(square: (8, 1), time: 2.751),
    .init(square: (8, 2), time: 3.907),
    .init(square: (8, 3), time: 5.063),
    .init(square: (8, 4), time: 6.285),
    .init(square: (8, 5), time: 7.335),
    .init(square: (8, 6), time: 8.438),
    .init(square: (8, 7), time: 9.501),
    .init(square: (8, 8), time: 10.683),

    .init(square: (7, 0), time: 11.560),
    .init(square: (7, 1), time: 12.690),
    .init(square: (7, 2), time: 13.700),
    .init(square: (7, 3), time: 14.670),
    .init(square: (7, 4), time: 15.693),
    .init(square: (7, 5), time: 16.596),
    .init(square: (7, 6), time: 17.580),
    .init(square: (7, 7), time: 17.563),
    .init(square: (7, 8), time: 19.626),

    .init(square: (6, 0), time: 20.476),
    .init(square: (6, 1), time: 21.579),
    .init(square: (6, 2), time: 22.682),
    .init(square: (6, 3), time: 23.652),
    .init(square: (6, 4), time: 24.728),
    .init(square: (6, 5), time: 25.672),
    .init(square: (6, 6), time: 26.721),
    .init(square: (6, 7), time: 27.758),
    .init(square: (6, 8), time: 28.874),

    .init(square: (5, 0), time: 29.818),
    .init(square: (5, 1), time: 30.967),
    .init(square: (5, 2), time: 31.904),
    .init(square: (5, 3), time: 32.860),
    .init(square: (5, 4), time: 33.897),
    .init(square: (5, 5), time: 34.787),
    .init(square: (5, 6), time: 35.784),
    .init(square: (5, 7), time: 36.754),
    .init(square: (5, 8), time: 37.830),

    .init(square: (4, 0), time: 38.680),
    .init(square: (4, 1), time: 39.690),
    .init(square: (4, 2), time: 40.660),
    .init(square: (4, 3), time: 41.551),
    .init(square: (4, 4), time: 42.521),
    .init(square: (4, 5), time: 43.384),
    .init(square: (4, 6), time: 44.274),
    .init(square: (4, 7), time: 45.178),
    .init(square: (4, 8), time: 46.175),

    .init(square: (3, 0), time: 47.025),
    .init(square: (3, 1), time: 48.101),
    .init(square: (3, 2), time: 49.124),
    .init(square: (3, 3), time: 50.081),
    .init(square: (3, 4), time: 51.104),
    .init(square: (3, 5), time: 51.995),
    .init(square: (3, 6), time: 52.965),
    .init(square: (3, 7), time: 53.895),
    .init(square: (3, 8), time: 54.905),

    .init(square: (2, 0), time: 55.821),
    .init(square: (2, 1), time: 56.884),
    .init(square: (2, 2), time: 57.947),
    .init(square: (2, 3), time: 58.944),
    .init(square: (2, 4), time: 59.980),
    .init(square: (2, 5), time: 60.964),
    .init(square: (2, 6), time: 61.974),
    .init(square: (2, 7), time: 62.957),
    .init(square: (2, 8), time: 64.007),

    .init(square: (1, 0), time: 64.897),
    .init(square: (1, 1), time: 66.026),
    .init(square: (1, 2), time: 67.089),
    .init(square: (1, 3), time: 68.113),
    .init(square: (1, 4), time: 69.176),
    .init(square: (1, 5), time: 70.172),
    .init(square: (1, 6), time: 71.209),
    .init(square: (1, 7), time: 72.245),
    .init(square: (1, 8), time: 73.321),

    .init(square: (0, 0), time: 74.331),
    .init(square: (0, 1), time: 75.474),
    .init(square: (0, 2), time: 76.577),
    .init(square: (0, 3), time: 77.560),
    .init(square: (0, 4), time: 78.636),
    .init(square: (0, 5), time: 79.593),
    .init(square: (0, 6), time: 80.643),
    .init(square: (0, 7), time: 81.666),
    .init(square: (0, 8), time: 82.756),

    .init(square: (9, 0), time: 83.712),
  ]

  struct Piece {
    let piece: UInt32
    let time: TimeInterval
  }
  private let pieces: [Piece] = [
    .init(piece: PieceType.Pawn.rawValue, time: 83.739),
    .init(piece: PieceType.Lance.rawValue, time: 84.310),
    .init(piece: PieceType.Knight.rawValue, time: 85.094),
    .init(piece: PieceType.Silver.rawValue, time: 85.918),
    .init(piece: PieceType.Gold.rawValue, time: 86.542),
    .init(piece: PieceType.Bishop.rawValue, time: 87.061),
    .init(piece: PieceType.Rook.rawValue, time: 87.738),
    .init(piece: PieceType.King.rawValue, time: 88.456),
    .init(piece: PieceType.Pawn.rawValue + PieceStatus.Promoted.rawValue, time: 89.173),
    .init(piece: PieceType.Lance.rawValue + PieceStatus.Promoted.rawValue, time: 89.904),
    .init(piece: PieceType.Knight.rawValue + PieceStatus.Promoted.rawValue, time: 90.834),
    .init(piece: PieceType.Silver.rawValue + PieceStatus.Promoted.rawValue, time: 91.725),
    .init(piece: PieceType.Bishop.rawValue + PieceStatus.Promoted.rawValue, time: 92.602),
    .init(piece: PieceType.Rook.rawValue + PieceStatus.Promoted.rawValue, time: 93.266),
    .init(piece: 9999, time: 93.266),
  ]
  enum Action: UInt32 {
    case Promote
    case NoPromote
    case Right
    case Left
    case Up
    case Down
    case Lateral  // 寄る
    case Nearest  // 直ぐ
    case Drop  // 打ち
  }
  struct ActionVoice {
    let action: UInt32
    let time: TimeInterval
  }
  private let actions: [ActionVoice] = [
    .init(action: Action.Promote.rawValue, time: 93.903),
    .init(action: Action.NoPromote.rawValue, time: 94.513),
    .init(action: Action.Right.rawValue, time: 95.293),
    .init(action: Action.Left.rawValue, time: 95.931),
    .init(action: Action.Up.rawValue, time: 96.711),
    .init(action: Action.Down.rawValue, time: 97.434),
    .init(action: Action.Lateral.rawValue, time: 98.073),
    .init(action: Action.Nearest.rawValue, time: 98.654),
    .init(action: 9999, time: 99.236),
  ]

  func play(move: Move) {
    let startColor: Double
    let endColor: Double
    if move.color == .Black {
      startColor = 0
      endColor = 0.864
    } else {
      startColor = 0.864
      endColor = 1.648
    }
    let rate = file.fileFormat.sampleRate
    node.scheduleSegment(
      file, startingFrame: .init(startColor * rate),
      frameCount: .init((endColor - startColor) * rate), at: nil)
    var offset = endColor - startColor

    var startSquare: TimeInterval?
    var endSquare: TimeInterval?
    for i in 0..<self.positions.count - 1 {
      let p = self.positions[i]
      if p.square.file == Int(move.to.file.rawValue) && p.square.rank == Int(move.to.rank.rawValue)
      {
        startSquare = p.time
        endSquare = self.positions[i + 1].time
        break
      }
    }
    if let startSquare, let endSquare {
      node.scheduleSegment(
        file, startingFrame: .init(offset + startSquare * rate),
        frameCount: .init((endSquare - startSquare) * rate), at: nil)
      offset += endSquare - startSquare
    } else {
      print("マス目読み上げ用ボイスが搭載されていない: file=", move.to.file.rawValue, "rank=", move.to.rank.rawValue)
    }

    let piece =
      move.promote_.value == true ? sci.PieceTypeFromPiece(move.piece).rawValue : move.piece
    var startPiece: TimeInterval?
    var endPiece: TimeInterval?
    for i in 0..<self.pieces.count - 1 {
      let p = self.pieces[i]
      if p.piece == sci.RemoveColorFromPiece(piece) {
        startPiece = p.time
        endPiece = self.pieces[i + 1].time
        break
      }
    }
    if let startPiece, let endPiece {
      node.scheduleSegment(
        file, startingFrame: .init(offset + startPiece * rate),
        frameCount: .init((endPiece - startPiece) * rate), at: nil)
      offset += endPiece - startPiece
    } else {
      print("駒読み上げ用ボイスが搭載されていない: ", move.piece)
    }
    if let promote = move.promote_.value {
      let action: Action = promote ? .Promote : .NoPromote
      var startAction: TimeInterval?
      var endAction: TimeInterval?
      for i in 0..<self.actions.count - 1 {
        let a = self.actions[i]
        if a.action == action.rawValue {
          startAction = a.time
          endAction = self.actions[i + 1].time
        }
      }
      if let startAction, let endAction {
        node.scheduleSegment(
          file, startingFrame: .init(offset + startAction * rate),
          frameCount: .init((endAction - startAction) * rate), at: nil)
        offset += endAction - startAction
      } else {
        print("アクション用のボイスが無い")
      }
    }
    if !move.from.__convertToBool() {
      let action: Action = .Drop
      var startAction: TimeInterval?
      var endAction: TimeInterval?
      for i in 0..<self.actions.count - 1 {
        let a = self.actions[i]
        if a.action == action.rawValue {
          startAction = a.time
          endAction = self.actions[i + 1].time
        }
      }
      if let startAction, let endAction {
        node.scheduleSegment(
          file, startingFrame: .init(offset + startAction * rate),
          frameCount: .init((endAction - startAction) * rate), at: nil)
        offset += endAction - startAction
      } else {
        print("アクション用のボイスが無い")
      }
    }
  }
}
