import AVFoundation
import AudioToolbox
import ShogiCamera

class Reader {
  private let engine = AVAudioEngine()
  private let node = AVAudioPlayerNode()
  private var file: AVAudioFile!
  // 読み上げ中のメッセージが読み終わる時刻
  private var latest: Date?

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
    case Ready
    case WrongMoveWarningLeading
    case WrongMoveWarningTrailing
    case YourTurnBlack
    case YourTurnWhite
    case Aborted
  }
  struct MiscVoice {
    let misc: UInt32
    let time: TimeInterval
  }

  private let positions: [Position] = [
    .init(square: (8, 0), time: 14.677333),
    .init(square: (8, 1), time: 15.616000),
    .init(square: (8, 2), time: 16.501333),
    .init(square: (8, 3), time: 17.344000),
    .init(square: (8, 4), time: 18.186667),
    .init(square: (8, 5), time: 19.050667),
    .init(square: (8, 6), time: 19.957334),
    .init(square: (8, 7), time: 20.842667),
    .init(square: (8, 8), time: 21.813334),
    .init(square: (7, 0), time: 22.709334),
    .init(square: (7, 1), time: 23.669334),
    .init(square: (7, 2), time: 24.554667),
    .init(square: (7, 3), time: 25.386667),
    .init(square: (7, 4), time: 26.229334),
    .init(square: (7, 5), time: 27.093334),
    .init(square: (7, 6), time: 28.000001),
    .init(square: (7, 7), time: 28.885334),
    .init(square: (7, 8), time: 29.856001),
    .init(square: (6, 0), time: 30.752001),
    .init(square: (6, 1), time: 31.722668),
    .init(square: (6, 2), time: 32.640001),
    .init(square: (6, 3), time: 33.482668),
    .init(square: (6, 4), time: 34.336001),
    .init(square: (6, 5), time: 35.242668),
    .init(square: (6, 6), time: 36.192001),
    .init(square: (6, 7), time: 37.120001),
    .init(square: (6, 8), time: 38.101334),
    .init(square: (5, 0), time: 39.018667),
    .init(square: (5, 1), time: 39.957334),
    .init(square: (5, 2), time: 40.800001),
    .init(square: (5, 3), time: 41.589334),
    .init(square: (5, 4), time: 42.378667),
    .init(square: (5, 5), time: 43.232000),
    .init(square: (5, 6), time: 44.117333),
    .init(square: (5, 7), time: 44.981333),
    .init(square: (5, 8), time: 45.898666),
    .init(square: (4, 0), time: 46.741333),
    .init(square: (4, 1), time: 47.680000),
    .init(square: (4, 2), time: 48.533333),
    .init(square: (4, 3), time: 49.365333),
    .init(square: (4, 4), time: 50.154666),
    .init(square: (4, 5), time: 50.997333),
    .init(square: (4, 6), time: 51.861333),
    .init(square: (4, 7), time: 52.714666),
    .init(square: (4, 8), time: 53.653333),
    .init(square: (3, 0), time: 54.549333),
    .init(square: (3, 1), time: 55.498666),
    .init(square: (3, 2), time: 56.351999),
    .init(square: (3, 3), time: 57.183999),
    .init(square: (3, 4), time: 58.005332),
    .init(square: (3, 5), time: 58.858665),
    .init(square: (3, 6), time: 59.786665),
    .init(square: (3, 7), time: 60.693332),
    .init(square: (3, 8), time: 61.663999),
    .init(square: (2, 0), time: 62.559999),
    .init(square: (2, 1), time: 63.487999),
    .init(square: (2, 2), time: 64.362666),
    .init(square: (2, 3), time: 65.205333),
    .init(square: (2, 4), time: 66.048000),
    .init(square: (2, 5), time: 66.933333),
    .init(square: (2, 6), time: 67.861333),
    .init(square: (2, 7), time: 68.768000),
    .init(square: (2, 8), time: 69.738667),
    .init(square: (1, 0), time: 70.656000),
    .init(square: (1, 1), time: 71.626667),
    .init(square: (1, 2), time: 72.544000),
    .init(square: (1, 3), time: 73.429333),
    .init(square: (1, 4), time: 74.304000),
    .init(square: (1, 5), time: 75.232000),
    .init(square: (1, 6), time: 76.202667),
    .init(square: (1, 7), time: 77.152000),
    .init(square: (1, 8), time: 78.197333),
    .init(square: (0, 0), time: 79.136000),
    .init(square: (0, 1), time: 80.149333),
    .init(square: (0, 2), time: 81.088000),
    .init(square: (0, 3), time: 81.973333),
    .init(square: (0, 4), time: 82.858666),
    .init(square: (0, 5), time: 83.775999),
    .init(square: (0, 6), time: 84.746666),
    .init(square: (0, 7), time: 85.674666),
    .init(square: (0, 8), time: 86.677333),
    .init(square: (Int.max, Int.max), time: 87.605333),
  ]
  private let pieces: [Piece] = [
    .init(piece: PieceType.Pawn.rawValue, time: 87.605333),
    .init(piece: PieceType.Lance.rawValue, time: 88.213333),
    .init(piece: PieceType.Knight.rawValue, time: 89.002666),
    .init(piece: PieceType.Silver.rawValue, time: 89.823999),
    .init(piece: PieceType.Gold.rawValue, time: 90.399999),
    .init(piece: PieceType.Bishop.rawValue, time: 90.997332),
    .init(piece: PieceType.Rook.rawValue, time: 91.690665),
    .init(piece: PieceType.King.rawValue, time: 92.394665),
    .init(piece: PieceType.Pawn.rawValue + PieceStatus.Promoted.rawValue, time: 93.087998),
    .init(piece: PieceType.Lance.rawValue + PieceStatus.Promoted.rawValue, time: 93.845331),
    .init(piece: PieceType.Knight.rawValue + PieceStatus.Promoted.rawValue, time: 94.762664),
    .init(piece: PieceType.Silver.rawValue + PieceStatus.Promoted.rawValue, time: 95.679997),
    .init(piece: PieceType.Bishop.rawValue + PieceStatus.Promoted.rawValue, time: 96.543997),
    .init(piece: PieceType.Rook.rawValue + PieceStatus.Promoted.rawValue, time: 97.162664),
    .init(piece: UInt32.max, time: 97.813331),
  ]
  private let takes: [Piece] = [
    .init(piece: PieceType.Pawn.rawValue, time: 97.813331),
    .init(piece: PieceType.Lance.rawValue, time: 98.581331),
    .init(piece: PieceType.Knight.rawValue, time: 99.455998),
    .init(piece: PieceType.Silver.rawValue, time: 100.319998),
    .init(piece: PieceType.Gold.rawValue, time: 101.141331),
    .init(piece: PieceType.Bishop.rawValue, time: 101.994664),
    .init(piece: PieceType.Rook.rawValue, time: 102.879997),
    .init(piece: PieceType.King.rawValue, time: 103.807997),
    .init(piece: PieceType.Pawn.rawValue + PieceStatus.Promoted.rawValue, time: 104.714664),
    .init(piece: PieceType.Lance.rawValue + PieceStatus.Promoted.rawValue, time: 105.471997),
    .init(piece: PieceType.Knight.rawValue + PieceStatus.Promoted.rawValue, time: 106.623997),
    .init(piece: PieceType.Silver.rawValue + PieceStatus.Promoted.rawValue, time: 107.765330),
    .init(piece: PieceType.Bishop.rawValue + PieceStatus.Promoted.rawValue, time: 108.863997),
    .init(piece: PieceType.Rook.rawValue + PieceStatus.Promoted.rawValue, time: 109.706664),
    .init(piece: UInt32.max, time: 110.538664),
  ]
  private let actions: [ActionVoice] = [
    .init(action: Action.Promote.rawValue, time: 110.538664),
    .init(action: Action.NoPromote.rawValue, time: 111.178664),
    .init(action: Action.Drop.rawValue, time: 111.946664),
    .init(action: Action.Right.rawValue, time: 112.650664),
    .init(action: Action.Left.rawValue, time: 113.322664),
    .init(action: Action.Up.rawValue, time: 114.101331),
    .init(action: Action.Down.rawValue, time: 114.805331),
    .init(action: Action.Sideway.rawValue, time: 115.434664),
    .init(action: Action.Nearest.rawValue, time: 116.042664),
    .init(action: UInt32.max, time: 116.725331),
  ]
  private let miscs: [MiscVoice] = [
    .init(misc: Misc.Black.rawValue, time: 0),
    .init(misc: Misc.White.rawValue, time: 0.821333),
    .init(misc: Misc.Resign.rawValue, time: 1.536000),
    .init(misc: Misc.Ready.rawValue, time: 2.549333),
    .init(misc: Misc.WrongMoveWarningLeading.rawValue, time: 5.888000),
    .init(misc: Misc.WrongMoveWarningTrailing.rawValue, time: 9.632000),
    .init(misc: Misc.YourTurnBlack.rawValue, time: 10.240000),
    .init(misc: Misc.YourTurnWhite.rawValue, time: 11.765333),
    .init(misc: Misc.Aborted.rawValue, time: 13.162666),
    .init(misc: UInt32.max, time: 14.677333),
  ]

  private var availableOffset: TimeInterval {
    if let latest {
      let now = Date.now
      let delta = latest.timeIntervalSince(now)
      return max(0, delta)
    } else {
      return 0
    }
  }

  func playWrongMoveWarning(expected: Move, last: Move?) {
    var offset = schedule(misc: .WrongMoveWarningLeading, offset: availableOffset)
    offset = schedule(move: expected, last: last, offset: offset)
    offset = schedule(misc: .WrongMoveWarningTrailing, offset: offset)
    updateLatestAfter(seconds: offset)
  }

  func playReady() {
    let offset = schedule(misc: .Ready, offset: availableOffset)
    updateLatestAfter(seconds: offset)
  }

  func playYourTurn(_ color: sci.Color) {
    let offset = schedule(misc: color == sci.Color.Black ? .YourTurnBlack : .YourTurnWhite, offset: availableOffset)
    updateLatestAfter(seconds: offset)
  }

  func playNextMoveReady() {
    // ct-keytone2
    AudioServicesPlaySystemSound(1075)
  }

  func playAborted() {
    let offset = schedule(misc: .Aborted, offset: availableOffset)
    updateLatestAfter(seconds: offset)
  }

  private func updateLatestAfter(seconds: TimeInterval) {
    latest = Date.now.addingTimeInterval(seconds)
  }

  @discardableResult
  private func schedule(misc: Misc, offset startOffset: TimeInterval) -> TimeInterval {
    let rate = file.fileFormat.sampleRate
    for i in 0..<miscs.count - 1 {
      if miscs[i].misc == misc.rawValue {
        let start = miscs[i].time
        let end = miscs[i + 1].time
        node.scheduleSegment(
          file, startingFrame: .init(startOffset + start * rate),
          frameCount: .init((end - start) * rate), at: nil)
        return startOffset + (end - start)
      }
    }
    return startOffset
  }

  func playResign() {
    let offset = schedule(misc: .Resign, offset: availableOffset)
    updateLatestAfter(seconds: offset)
  }

  @discardableResult
  private func schedule(move: Move, last: Move?, offset startOffset: TimeInterval) -> TimeInterval {
    var offset = startOffset
    let rate = file.fileFormat.sampleRate
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
    return offset
  }

  func play(move: Move, last: Move?) {
    var offset = schedule(
      misc: move.color == .Black ? Misc.Black : Misc.White, offset: availableOffset)
    offset = schedule(move: move, last: last, offset: offset)
    updateLatestAfter(seconds: offset)
  }
}
