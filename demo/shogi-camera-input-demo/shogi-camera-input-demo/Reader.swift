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
      try session.setCategory(.playback, mode: .default)
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

  struct Piece {
    let piece: UInt32
    let time: TimeInterval
  }

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

  private let positions: [Position] = [
    .init(square: (8, 0), time: 1.536000),
    .init(square: (8, 1), time: 2.474667),
    .init(square: (8, 2), time: 3.360000),
    .init(square: (8, 3), time: 4.202667),
    .init(square: (8, 4), time: 5.045334),
    .init(square: (8, 5), time: 5.909334),
    .init(square: (8, 6), time: 6.816001),
    .init(square: (8, 7), time: 7.701334),
    .init(square: (8, 8), time: 8.672001),
    .init(square: (7, 0), time: 9.568001),
    .init(square: (7, 1), time: 10.528001),
    .init(square: (7, 2), time: 11.413334),
    .init(square: (7, 3), time: 12.245334),
    .init(square: (7, 4), time: 13.088001),
    .init(square: (7, 5), time: 13.952001),
    .init(square: (7, 6), time: 14.858668),
    .init(square: (7, 7), time: 15.744001),
    .init(square: (7, 8), time: 16.714668),
    .init(square: (6, 0), time: 17.610668),
    .init(square: (6, 1), time: 18.581335),
    .init(square: (6, 2), time: 19.498668),
    .init(square: (6, 3), time: 20.341335),
    .init(square: (6, 4), time: 21.194668),
    .init(square: (6, 5), time: 22.101335),
    .init(square: (6, 6), time: 23.050668),
    .init(square: (6, 7), time: 23.978668),
    .init(square: (6, 8), time: 24.960001),
    .init(square: (5, 0), time: 25.877334),
    .init(square: (5, 1), time: 26.816001),
    .init(square: (5, 2), time: 27.658668),
    .init(square: (5, 3), time: 28.448001),
    .init(square: (5, 4), time: 29.237334),
    .init(square: (5, 5), time: 30.090667),
    .init(square: (5, 6), time: 30.976000),
    .init(square: (5, 7), time: 31.840000),
    .init(square: (5, 8), time: 32.757333),
    .init(square: (4, 0), time: 33.600000),
    .init(square: (4, 1), time: 34.538667),
    .init(square: (4, 2), time: 35.392000),
    .init(square: (4, 3), time: 36.224000),
    .init(square: (4, 4), time: 37.013333),
    .init(square: (4, 5), time: 37.856000),
    .init(square: (4, 6), time: 38.720000),
    .init(square: (4, 7), time: 39.573333),
    .init(square: (4, 8), time: 40.512000),
    .init(square: (3, 0), time: 41.408000),
    .init(square: (3, 1), time: 42.357333),
    .init(square: (3, 2), time: 43.210666),
    .init(square: (3, 3), time: 44.042666),
    .init(square: (3, 4), time: 44.863999),
    .init(square: (3, 5), time: 45.717332),
    .init(square: (3, 6), time: 46.645332),
    .init(square: (3, 7), time: 47.551999),
    .init(square: (3, 8), time: 48.522666),
    .init(square: (2, 0), time: 49.418666),
    .init(square: (2, 1), time: 50.346666),
    .init(square: (2, 2), time: 51.221333),
    .init(square: (2, 3), time: 52.064000),
    .init(square: (2, 4), time: 52.906667),
    .init(square: (2, 5), time: 53.792000),
    .init(square: (2, 6), time: 54.720000),
    .init(square: (2, 7), time: 55.626667),
    .init(square: (2, 8), time: 56.597334),
    .init(square: (1, 0), time: 57.514667),
    .init(square: (1, 1), time: 58.485334),
    .init(square: (1, 2), time: 59.402667),
    .init(square: (1, 3), time: 60.288000),
    .init(square: (1, 4), time: 61.162667),
    .init(square: (1, 5), time: 62.090667),
    .init(square: (1, 6), time: 63.061334),
    .init(square: (1, 7), time: 64.010667),
    .init(square: (1, 8), time: 65.056000),
    .init(square: (0, 0), time: 65.994667),
    .init(square: (0, 1), time: 67.008000),
    .init(square: (0, 2), time: 67.946667),
    .init(square: (0, 3), time: 68.832000),
    .init(square: (0, 4), time: 69.717333),
    .init(square: (0, 5), time: 70.634666),
    .init(square: (0, 6), time: 71.605333),
    .init(square: (0, 7), time: 72.533333),
    .init(square: (0, 8), time: 73.536000),
    .init(square: (9, 0), time: 74.464000),
  ]
  private let pieces: [Piece] = [
    .init(piece: PieceType.Pawn.rawValue, time: 74.464000),
    .init(piece: PieceType.Lance.rawValue, time: 75.072000),
    .init(piece: PieceType.Knight.rawValue, time: 75.861333),
    .init(piece: PieceType.Silver.rawValue, time: 76.682666),
    .init(piece: PieceType.Gold.rawValue, time: 77.258666),
    .init(piece: PieceType.Bishop.rawValue, time: 77.855999),
    .init(piece: PieceType.Rook.rawValue, time: 78.549332),
    .init(piece: PieceType.King.rawValue, time: 79.253332),
    .init(piece: PieceType.Pawn.rawValue + PieceStatus.Promoted.rawValue, time: 79.946665),
    .init(piece: PieceType.Lance.rawValue + PieceStatus.Promoted.rawValue, time: 80.703998),
    .init(piece: PieceType.Knight.rawValue + PieceStatus.Promoted.rawValue, time: 81.621331),
    .init(piece: PieceType.Silver.rawValue + PieceStatus.Promoted.rawValue, time: 82.538664),
    .init(piece: PieceType.Bishop.rawValue + PieceStatus.Promoted.rawValue, time: 83.402664),
    .init(piece: PieceType.Rook.rawValue + PieceStatus.Promoted.rawValue, time: 84.021331),
    .init(piece: 9999, time: 84.021331),
  ]
  private let takes: [Piece] = [
    .init(piece: PieceType.Pawn.rawValue, time: 84.671998),
    .init(piece: PieceType.Lance.rawValue, time: 85.439998),
    .init(piece: PieceType.Knight.rawValue, time: 86.314665),
    .init(piece: PieceType.Silver.rawValue, time: 87.178665),
    .init(piece: PieceType.Gold.rawValue, time: 87.999998),
    .init(piece: PieceType.Bishop.rawValue, time: 88.853331),
    .init(piece: PieceType.Rook.rawValue, time: 89.738664),
    .init(piece: PieceType.King.rawValue, time: 90.666664),
    .init(piece: PieceType.Pawn.rawValue + PieceStatus.Promoted.rawValue, time: 91.573331),
    .init(piece: PieceType.Lance.rawValue + PieceStatus.Promoted.rawValue, time: 92.330664),
    .init(piece: PieceType.Knight.rawValue + PieceStatus.Promoted.rawValue, time: 93.482664),
    .init(piece: PieceType.Silver.rawValue + PieceStatus.Promoted.rawValue, time: 94.623997),
    .init(piece: PieceType.Bishop.rawValue + PieceStatus.Promoted.rawValue, time: 95.722664),
    .init(piece: PieceType.Rook.rawValue + PieceStatus.Promoted.rawValue, time: 96.565331),
    .init(piece: 9999, time: 96.565331),
  ]
  private let actions: [ActionVoice] = [
    .init(action: Action.Promote.rawValue, time: 97.397331),
    .init(action: Action.NoPromote.rawValue, time: 98.037331),
    .init(action: Action.Drop.rawValue, time: 98.805331),
    .init(action: Action.Right.rawValue, time: 99.509331),
    .init(action: Action.Left.rawValue, time: 100.181331),
    .init(action: Action.Up.rawValue, time: 100.959998),
    .init(action: Action.Down.rawValue, time: 101.663998),
    .init(action: Action.Lateral.rawValue, time: 102.293331),
    .init(action: Action.Nearest.rawValue, time: 102.901331),
    .init(action: 9999, time: 103.583998),
  ]

  func play(move: Move, last: Move?) {
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

    if let last, last.to == move.to {
      var startPiece: TimeInterval?
      var endPiece: TimeInterval?
      let piece =
        move.promote == 1 ? sci.PieceTypeFromPiece(move.piece).rawValue : move.piece
      for i in 0..<self.takes.count - 1 {
        let p = self.takes[i]
        if p.piece == sci.RemoveColorFromPiece(piece) {
          startPiece = p.time
          endPiece = self.takes[i + 1].time
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
    } else {
      var startSquare: TimeInterval?
      var endSquare: TimeInterval?
      for i in 0..<self.positions.count - 1 {
        let p = self.positions[i]
        if p.square.file == Int(move.to.file.rawValue)
          && p.square.rank == Int(move.to.rank.rawValue)
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
        move.promote == 1 ? sci.PieceTypeFromPiece(move.piece).rawValue : move.piece
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
    }
    if move.promote != 0 {
      let action: Action = move.promote == 1 ? .Promote : .NoPromote
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
