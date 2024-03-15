#include <shogi_camera/shogi_camera.hpp>

using namespace std;

namespace sci {

struct CsaAdapter::Impl {
  Impl(string const &server, int port, string const &username, string const &password) {
  }

  optional<Move> next(Position const &p, vector<Move> const &moves, deque<PieceType> const &hand, deque<PieceType> const &handEnemy) {
    return nullopt;
  }
};

CsaAdapter::CsaAdapter(string const &server, int port, string const &username, string const &password) : impl(make_unique<Impl>(server, port, username, password)) {
}

CsaAdapter::~CsaAdapter() {
}

optional<Move> CsaAdapter::next(Position const &p, vector<Move> const &moves, deque<PieceType> const &hand, deque<PieceType> const &handEnemy) {
  return impl->next(p, moves, hand, handEnemy);
}

} // namespace sci
