#include <shogi_camera/shogi_camera.hpp>

using namespace std;

namespace sci {

RandomAI::RandomAI() {
  random_device seed_gen;
  engine = make_unique<mt19937_64>(seed_gen());
}

optional<Move> RandomAI::next(Position const &p, Color next, vector<Move> const &, deque<PieceType> const &hand, deque<PieceType> const &handEnemy) {
  deque<Move> moves;
  Game::Generate(p, next, next == Color::Black ? hand : handEnemy, next == Color::Black ? handEnemy : hand, moves, true);
  if (moves.empty()) {
    return nullopt;
  } else if (moves.size() == 1) {
    return moves[0];
  } else {
    uniform_int_distribution<int> dist(0, (int)moves.size() - 1);
    int index = dist(*engine);
    return moves[index];
  }
}

} // namespace sci
