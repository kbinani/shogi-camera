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
    case Sideway  // 寄る
    case Nearest  // 直ぐ
    case Drop  // 打ち
  }

  struct ActionVoice {
    let action: UInt32
    let time: TimeInterval
  }

  enum Misc: UInt32 {
    case Black
    case White
    case Resign
  }
  struct MiscVoice {
    let misc: UInt32
    let time: TimeInterval
  }

  private let positions: [Position] = [
    .init(square: (8, 0), time: 2.549333),
    .init(square: (8, 1), time: 3.488000),
    .init(square: (8, 2), time: 4.373333),
    .init(square: (8, 3), time: 5.216000),
    .init(square: (8, 4), time: 6.058667),
    .init(square: (8, 5), time: 6.922667),
    .init(square: (8, 6), time: 7.829334),
    .init(square: (8, 7), time: 8.714667),
    .init(square: (8, 8), time: 9.685334),
    .init(square: (7, 0), time: 10.581334),
    .init(square: (7, 1), time: 11.541334),
    .init(square: (7, 2), time: 12.426667),
    .init(square: (7, 3), time: 13.258667),
    .init(square: (7, 4), time: 14.101334),
    .init(square: (7, 5), time: 14.965334),
    .init(square: (7, 6), time: 15.872001),
    .init(square: (7, 7), time: 16.757334),
    .init(square: (7, 8), time: 17.728001),
    .init(square: (6, 0), time: 18.624001),
    .init(square: (6, 1), time: 19.594668),
    .init(square: (6, 2), time: 20.512001),
    .init(square: (6, 3), time: 21.354668),
    .init(square: (6, 4), time: 22.208001),
    .init(square: (6, 5), time: 23.114668),
    .init(square: (6, 6), time: 24.064001),
    .init(square: (6, 7), time: 24.992001),
    .init(square: (6, 8), time: 25.973334),
    .init(square: (5, 0), time: 26.890667),
    .init(square: (5, 1), time: 27.829334),
    .init(square: (5, 2), time: 28.672001),
    .init(square: (5, 3), time: 29.461334),
    .init(square: (5, 4), time: 30.250667),
    .init(square: (5, 5), time: 31.104000),
    .init(square: (5, 6), time: 31.989333),
    .init(square: (5, 7), time: 32.853333),
    .init(square: (5, 8), time: 33.770666),
    .init(square: (4, 0), time: 34.613333),
    .init(square: (4, 1), time: 35.552000),
    .init(square: (4, 2), time: 36.405333),
    .init(square: (4, 3), time: 37.237333),
    .init(square: (4, 4), time: 38.026666),
    .init(square: (4, 5), time: 38.869333),
    .init(square: (4, 6), time: 39.733333),
    .init(square: (4, 7), time: 40.586666),
    .init(square: (4, 8), time: 41.525333),
    .init(square: (3, 0), time: 42.421333),
    .init(square: (3, 1), time: 43.370666),
    .init(square: (3, 2), time: 44.223999),
    .init(square: (3, 3), time: 45.055999),
    .init(square: (3, 4), time: 45.877332),
    .init(square: (3, 5), time: 46.730665),
    .init(square: (3, 6), time: 47.658665),
    .init(square: (3, 7), time: 48.565332),
    .init(square: (3, 8), time: 49.535999),
    .init(square: (2, 0), time: 50.431999),
    .init(square: (2, 1), time: 51.359999),
    .init(square: (2, 2), time: 52.234666),
    .init(square: (2, 3), time: 53.077333),
    .init(square: (2, 4), time: 53.920000),
    .init(square: (2, 5), time: 54.805333),
    .init(square: (2, 6), time: 55.733333),
    .init(square: (2, 7), time: 56.640000),
    .init(square: (2, 8), time: 57.610667),
    .init(square: (1, 0), time: 58.528000),
    .init(square: (1, 1), time: 59.498667),
    .init(square: (1, 2), time: 60.416000),
    .init(square: (1, 3), time: 61.301333),
    .init(square: (1, 4), time: 62.176000),
    .init(square: (1, 5), time: 63.104000),
    .init(square: (1, 6), time: 64.074667),
    .init(square: (1, 7), time: 65.024000),
    .init(square: (1, 8), time: 66.069333),
    .init(square: (0, 0), time: 67.008000),
    .init(square: (0, 1), time: 68.021333),
    .init(square: (0, 2), time: 68.960000),
    .init(square: (0, 3), time: 69.845333),
    .init(square: (0, 4), time: 70.730666),
    .init(square: (0, 5), time: 71.647999),
    .init(square: (0, 6), time: 72.618666),
    .init(square: (0, 7), time: 73.546666),
    .init(square: (0, 8), time: 74.549333),
    .init(square: (Int.max, Int.max), time: 75.477333),
  ]
  private let pieces: [Piece] = [
    .init(piece: PieceType.Pawn.rawValue, time: 75.477333),
    .init(piece: PieceType.Lance.rawValue, time: 76.085333),
    .init(piece: PieceType.Knight.rawValue, time: 76.874666),
    .init(piece: PieceType.Silver.rawValue, time: 77.695999),
    .init(piece: PieceType.Gold.rawValue, time: 78.271999),
    .init(piece: PieceType.Bishop.rawValue, time: 78.869332),
    .init(piece: PieceType.Rook.rawValue, time: 79.562665),
    .init(piece: PieceType.King.rawValue, time: 80.266665),
    .init(piece: PieceType.Pawn.rawValue + PieceStatus.Promoted.rawValue, time: 80.959998),
    .init(piece: PieceType.Lance.rawValue + PieceStatus.Promoted.rawValue, time: 81.717331),
    .init(piece: PieceType.Knight.rawValue + PieceStatus.Promoted.rawValue, time: 82.634664),
    .init(piece: PieceType.Silver.rawValue + PieceStatus.Promoted.rawValue, time: 83.551997),
    .init(piece: PieceType.Bishop.rawValue + PieceStatus.Promoted.rawValue, time: 84.415997),
    .init(piece: PieceType.Rook.rawValue + PieceStatus.Promoted.rawValue, time: 85.034664),
    .init(piece: UInt32.max, time: 85.685331),
  ]
  private let takes: [Piece] = [
    .init(piece: PieceType.Pawn.rawValue, time: 85.685331),
    .init(piece: PieceType.Lance.rawValue, time: 86.453331),
    .init(piece: PieceType.Knight.rawValue, time: 87.327998),
    .init(piece: PieceType.Silver.rawValue, time: 88.191998),
    .init(piece: PieceType.Gold.rawValue, time: 89.013331),
    .init(piece: PieceType.Bishop.rawValue, time: 89.866664),
    .init(piece: PieceType.Rook.rawValue, time: 90.751997),
    .init(piece: PieceType.King.rawValue, time: 91.679997),
    .init(piece: PieceType.Pawn.rawValue + PieceStatus.Promoted.rawValue, time: 92.586664),
    .init(piece: PieceType.Lance.rawValue + PieceStatus.Promoted.rawValue, time: 93.343997),
    .init(piece: PieceType.Knight.rawValue + PieceStatus.Promoted.rawValue, time: 94.495997),
    .init(piece: PieceType.Silver.rawValue + PieceStatus.Promoted.rawValue, time: 95.637330),
    .init(piece: PieceType.Bishop.rawValue + PieceStatus.Promoted.rawValue, time: 96.735997),
    .init(piece: PieceType.Rook.rawValue + PieceStatus.Promoted.rawValue, time: 97.578664),
    .init(piece: UInt32.max, time: 98.410664),
  ]
  private let actions: [ActionVoice] = [
    .init(action: Action.Promote.rawValue, time: 98.410664),
    .init(action: Action.NoPromote.rawValue, time: 99.050664),
    .init(action: Action.Drop.rawValue, time: 99.818664),
    .init(action: Action.Right.rawValue, time: 100.522664),
    .init(action: Action.Left.rawValue, time: 101.194664),
    .init(action: Action.Up.rawValue, time: 101.973331),
    .init(action: Action.Down.rawValue, time: 102.677331),
    .init(action: Action.Sideway.rawValue, time: 103.306664),
    .init(action: Action.Nearest.rawValue, time: 103.914664),
    .init(action: UInt32.max, time: 104.597331),
  ]
  private let miscs: [MiscVoice] = [
    .init(misc: Misc.Black.rawValue, time: 0),
    .init(misc: Misc.White.rawValue, time: 0.821333),
    .init(misc: Misc.Resign.rawValue, time: 1.536000),
    .init(misc: UInt32.max, time: 2.549333),
  ]

  func playResign() {
    let rate = file.fileFormat.sampleRate

    var startColor: Double?
    var endColor: Double?
    for i in 0..<miscs.count - 1 {
      if miscs[i].misc == Misc.Resign.rawValue {
        startColor = miscs[i].time
        endColor = miscs[i + 1].time
        break
      }
    }
    if let startColor, let endColor {
      node.scheduleSegment(
        file, startingFrame: .init(startColor * rate),
        frameCount: .init((endColor - startColor) * rate), at: nil)
    }
  }

  func play(move: Move, last: Move?) {
    let rate = file.fileFormat.sampleRate
    var offset: TimeInterval = 0

    var startColor: Double?
    var endColor: Double?
    let m = move.color == .Black ? Misc.Black : Misc.White
    for i in 0..<miscs.count - 1 {
      if miscs[i].misc == m.rawValue {
        startColor = miscs[i].time
        endColor = miscs[i + 1].time
        break
      }
    }
    if let startColor, let endColor {
      node.scheduleSegment(
        file, startingFrame: .init(offset + startColor * rate),
        frameCount: .init((endColor - startColor) * rate), at: nil)
      offset += endColor - startColor
    }

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
    var actions: [Action] = []
    if (move.suffix & sci.SuffixType.Right.rawValue) == sci.SuffixType.Right.rawValue {
      actions.append(.Right)
    }
    if (move.suffix & sci.SuffixType.Left.rawValue) == sci.SuffixType.Left.rawValue {
      actions.append(.Left)
    }
    if (move.suffix & sci.SuffixType.Nearest.rawValue) == sci.SuffixType.Nearest.rawValue {
      actions.append(.Nearest)
    }
    if (move.suffix & sci.SuffixType.Up.rawValue) == sci.SuffixType.Up.rawValue {
      actions.append(.Up)
    }
    if (move.suffix & sci.SuffixType.Down.rawValue) == sci.SuffixType.Down.rawValue {
      actions.append(.Down)
    }
    if (move.suffix & sci.SuffixType.Sideway.rawValue) == sci.SuffixType.Sideway.rawValue {
      actions.append(.Sideway)
    }
    for action in actions {
      var startAction: TimeInterval?
      var endAction: TimeInterval?
      for i in 0..<self.actions.count - 1 {
        let a = self.actions[i]
        if a.action == action.rawValue {
          startAction = a.time
          endAction = self.actions[i + 1].time
          break
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
