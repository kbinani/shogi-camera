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
    case YourTurnFirst
    case YourTurnSecond
    case Aborted
    case Error
    case Repetition
  }
  struct MiscVoice {
    let misc: UInt32
    let time: TimeInterval
  }

  private let positions: [Position] = [
    .init(square: (8, 0), time: 18.730666),
    .init(square: (8, 1), time: 19.669333),
    .init(square: (8, 2), time: 20.554666),
    .init(square: (8, 3), time: 21.397333),
    .init(square: (8, 4), time: 22.240000),
    .init(square: (8, 5), time: 23.104000),
    .init(square: (8, 6), time: 24.010667),
    .init(square: (8, 7), time: 24.896000),
    .init(square: (8, 8), time: 25.866667),
    .init(square: (7, 0), time: 26.762667),
    .init(square: (7, 1), time: 27.722667),
    .init(square: (7, 2), time: 28.608000),
    .init(square: (7, 3), time: 29.440000),
    .init(square: (7, 4), time: 30.282667),
    .init(square: (7, 5), time: 31.146667),
    .init(square: (7, 6), time: 32.053334),
    .init(square: (7, 7), time: 32.938667),
    .init(square: (7, 8), time: 33.909334),
    .init(square: (6, 0), time: 34.805334),
    .init(square: (6, 1), time: 35.776001),
    .init(square: (6, 2), time: 36.693334),
    .init(square: (6, 3), time: 37.536001),
    .init(square: (6, 4), time: 38.389334),
    .init(square: (6, 5), time: 39.296001),
    .init(square: (6, 6), time: 40.245334),
    .init(square: (6, 7), time: 41.173334),
    .init(square: (6, 8), time: 42.154667),
    .init(square: (5, 0), time: 43.072000),
    .init(square: (5, 1), time: 44.010667),
    .init(square: (5, 2), time: 44.853334),
    .init(square: (5, 3), time: 45.642667),
    .init(square: (5, 4), time: 46.432000),
    .init(square: (5, 5), time: 47.285333),
    .init(square: (5, 6), time: 48.170666),
    .init(square: (5, 7), time: 49.034666),
    .init(square: (5, 8), time: 49.951999),
    .init(square: (4, 0), time: 50.794666),
    .init(square: (4, 1), time: 51.733333),
    .init(square: (4, 2), time: 52.586666),
    .init(square: (4, 3), time: 53.418666),
    .init(square: (4, 4), time: 54.207999),
    .init(square: (4, 5), time: 55.050666),
    .init(square: (4, 6), time: 55.914666),
    .init(square: (4, 7), time: 56.767999),
    .init(square: (4, 8), time: 57.706666),
    .init(square: (3, 0), time: 58.602666),
    .init(square: (3, 1), time: 59.551999),
    .init(square: (3, 2), time: 60.405332),
    .init(square: (3, 3), time: 61.237332),
    .init(square: (3, 4), time: 62.058665),
    .init(square: (3, 5), time: 62.911998),
    .init(square: (3, 6), time: 63.839998),
    .init(square: (3, 7), time: 64.746665),
    .init(square: (3, 8), time: 65.717332),
    .init(square: (2, 0), time: 66.613332),
    .init(square: (2, 1), time: 67.541332),
    .init(square: (2, 2), time: 68.415999),
    .init(square: (2, 3), time: 69.258666),
    .init(square: (2, 4), time: 70.101333),
    .init(square: (2, 5), time: 70.986666),
    .init(square: (2, 6), time: 71.914666),
    .init(square: (2, 7), time: 72.821333),
    .init(square: (2, 8), time: 73.792000),
    .init(square: (1, 0), time: 74.709333),
    .init(square: (1, 1), time: 75.680000),
    .init(square: (1, 2), time: 76.597333),
    .init(square: (1, 3), time: 77.482666),
    .init(square: (1, 4), time: 78.357333),
    .init(square: (1, 5), time: 79.285333),
    .init(square: (1, 6), time: 80.256000),
    .init(square: (1, 7), time: 81.205333),
    .init(square: (1, 8), time: 82.250666),
    .init(square: (0, 0), time: 83.189333),
    .init(square: (0, 1), time: 84.202666),
    .init(square: (0, 2), time: 85.141333),
    .init(square: (0, 3), time: 86.026666),
    .init(square: (0, 4), time: 86.911999),
    .init(square: (0, 5), time: 87.829332),
    .init(square: (0, 6), time: 88.799999),
    .init(square: (0, 7), time: 89.727999),
    .init(square: (0, 8), time: 90.730666),
    .init(square: (Int.max, Int.max), time: 91.658666),
  ]
  private let pieces: [Piece] = [
    .init(piece: PieceType.Pawn.rawValue, time: 91.658666),
    .init(piece: PieceType.Lance.rawValue, time: 92.266666),
    .init(piece: PieceType.Knight.rawValue, time: 93.055999),
    .init(piece: PieceType.Silver.rawValue, time: 93.877332),
    .init(piece: PieceType.Gold.rawValue, time: 94.453332),
    .init(piece: PieceType.Bishop.rawValue, time: 95.050665),
    .init(piece: PieceType.Rook.rawValue, time: 95.743998),
    .init(piece: PieceType.King.rawValue, time: 96.447998),
    .init(piece: PieceType.Pawn.rawValue + PieceStatus.Promoted.rawValue, time: 97.141331),
    .init(piece: PieceType.Lance.rawValue + PieceStatus.Promoted.rawValue, time: 97.898664),
    .init(piece: PieceType.Knight.rawValue + PieceStatus.Promoted.rawValue, time: 98.815997),
    .init(piece: PieceType.Silver.rawValue + PieceStatus.Promoted.rawValue, time: 99.733330),
    .init(piece: PieceType.Bishop.rawValue + PieceStatus.Promoted.rawValue, time: 100.597330),
    .init(piece: PieceType.Rook.rawValue + PieceStatus.Promoted.rawValue, time: 101.215997),
    .init(piece: UInt32.max, time: 101.866664),
  ]
  private let takes: [Piece] = [
    .init(piece: PieceType.Pawn.rawValue, time: 101.866664),
    .init(piece: PieceType.Lance.rawValue, time: 102.634664),
    .init(piece: PieceType.Knight.rawValue, time: 103.509331),
    .init(piece: PieceType.Silver.rawValue, time: 104.373331),
    .init(piece: PieceType.Gold.rawValue, time: 105.194664),
    .init(piece: PieceType.Bishop.rawValue, time: 106.047997),
    .init(piece: PieceType.Rook.rawValue, time: 106.933330),
    .init(piece: PieceType.King.rawValue, time: 107.861330),
    .init(piece: PieceType.Pawn.rawValue + PieceStatus.Promoted.rawValue, time: 108.767997),
    .init(piece: PieceType.Lance.rawValue + PieceStatus.Promoted.rawValue, time: 109.525330),
    .init(piece: PieceType.Knight.rawValue + PieceStatus.Promoted.rawValue, time: 110.677330),
    .init(piece: PieceType.Silver.rawValue + PieceStatus.Promoted.rawValue, time: 111.818663),
    .init(piece: PieceType.Bishop.rawValue + PieceStatus.Promoted.rawValue, time: 112.917330),
    .init(piece: PieceType.Rook.rawValue + PieceStatus.Promoted.rawValue, time: 113.759997),
    .init(piece: UInt32.max, time: 114.591997),
  ]
  private let actions: [ActionVoice] = [
    .init(action: Action.Promote.rawValue, time: 114.591997),
    .init(action: Action.NoPromote.rawValue, time: 115.231997),
    .init(action: Action.Drop.rawValue, time: 115.999997),
    .init(action: Action.Right.rawValue, time: 116.703997),
    .init(action: Action.Left.rawValue, time: 117.375997),
    .init(action: Action.Up.rawValue, time: 118.154664),
    .init(action: Action.Down.rawValue, time: 118.858664),
    .init(action: Action.Sideway.rawValue, time: 119.487997),
    .init(action: Action.Nearest.rawValue, time: 120.095997),
    .init(action: UInt32.max, time: 120.778664),
  ]
  private let miscs: [MiscVoice] = [
    .init(misc: Misc.Black.rawValue, time: 0),
    .init(misc: Misc.White.rawValue, time: 0.821333),
    .init(misc: Misc.Resign.rawValue, time: 1.536000),
    .init(misc: Misc.Ready.rawValue, time: 2.549333),
    .init(misc: Misc.WrongMoveWarningLeading.rawValue, time: 5.888000),
    .init(misc: Misc.WrongMoveWarningTrailing.rawValue, time: 9.632000),
    .init(misc: Misc.YourTurnFirst.rawValue, time: 10.240000),
    .init(misc: Misc.YourTurnSecond.rawValue, time: 11.765333),
    .init(misc: Misc.Aborted.rawValue, time: 13.162666),
    .init(misc: Misc.Error.rawValue, time: 14.677333),
    .init(misc: Misc.Repetition.rawValue, time: 16.533333),
    .init(misc: UInt32.max, time: 18.730666),
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

  func playYourTurn(_ first: Bool) {
    let offset = schedule(misc: first ? .YourTurnFirst : .YourTurnSecond, offset: availableOffset)
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

  func playError() {
    let offset = schedule(misc: .Error, offset: availableOffset)
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
    let suffixPosition = move.suffix & sci.SuffixType.MaskPosition.rawValue
    let suffixAction = move.suffix & sci.SuffixType.MaskAction.rawValue
    if suffixPosition == sci.SuffixType.Right.rawValue {
      actions.append(.Right)
    } else if suffixPosition == sci.SuffixType.Left.rawValue {
      actions.append(.Left)
    } else if suffixPosition == sci.SuffixType.Nearest.rawValue {
      actions.append(.Nearest)
    }
    if suffixAction == sci.SuffixType.Up.rawValue {
      actions.append(.Up)
    } else if suffixAction == sci.SuffixType.Down.rawValue {
      actions.append(.Down)
    } else if suffixAction == sci.SuffixType.Sideway.rawValue {
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

  func play(move: Move, first: sci.Color, last: Move?) {
    let voice: Misc =
      if first == sci.Color.Black {
        move.color == .Black ? .Black : .White
      } else {
        move.color == .Black ? .White : .Black
      }
    var offset = schedule(misc: voice, offset: availableOffset)
    offset = schedule(move: move, last: last, offset: offset)
    updateLatestAfter(seconds: offset)
  }
}
