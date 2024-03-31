#include <shogi_camera/shogi_camera.hpp>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

using namespace std;

namespace sci {

namespace {

struct Match {
  Match(shared_ptr<CsaServer::Peer> local, Color localColor, int remoteSocket, string remoteUsername, Handicap h, bool hand)
      : local(local), localColor(localColor), remoteSocket(remoteSocket), remoteUsername(remoteUsername), game(h, hand) {
    gameId = "shogicamera";
    time = chrono::system_clock::now();
  }

  string seconds() const {
    auto now = chrono::system_clock::now();
    auto elapsed = chrono::duration_cast<chrono::duration<double>>(now - time).count();
    return "T" + to_string(std::max(0, (int)floor(elapsed)));
  }

  void update() {
    time = chrono::system_clock::now();
  }

  void sendBoth(string const &msg) {
    sendLocal(msg);
    sendRemote(msg);
  }

  void sendRemote(string const &line) {
    string m = line + "\x0a";
    ::send(remoteSocket, m.c_str(), m.size(), 0);
    cout << "csa_server:<<< " << line << endl;
  }

  void sendLocal(string const &msg) {
    local->onmessage(msg);
  }

  enum class Source {
    Remote,
    Local,
  };

  static Source OpponentSource(Source s) {
    switch (s) {
    case Source::Remote:
      return Source::Local;
    case Source::Local:
      return Source::Remote;
    }
  }

  void sendWin(Source win) {
    switch (win) {
    case Source::Remote:
      sendRemote("#WIN");
      sendLocal("#LOSE");
      break;
    case Source::Local:
      sendRemote("#LOSE");
      sendLocal("#WIN");
      break;
    }
  }

  void sendWin(Color color) {
    if (color == localColor) {
      sendLocal("#WIN");
      sendRemote("#LOSE");
    } else {
      sendLocal("#LOSE");
      sendRemote("#WIN");
    }
  }

  enum class HandleResult {
    Terminate,
    Continue,
  };
  HandleResult handle(string const &msg, Source d) {
    if (msg == "AGREE") {
      agrees++;
      if (agrees > 1) {
        start();
      }
    } else if (msg == "AGREE " + gameId) {
      agrees++;
      if (agrees > 1) {
        start();
      }
    } else if (msg == "REJECT") {
      switch (d) {
      case Source::Remote:
        sendLocal("REJECT:" + gameId + " by " + remoteUsername);
        break;
      case Source::Local:
        sendRemote("REJECT:" + gameId + " by " + local->name());
        break;
      }
      return HandleResult::Terminate;
    } else if (msg == "REJECT " + gameId) {
      switch (d) {
      case Source::Remote:
        sendLocal("REJECT:" + gameId + " by " + remoteUsername);
        break;
      case Source::Local:
        sendLocal("REJECT:" + gameId + " by " + local->name());
        break;
      }
      return HandleResult::Terminate;
    } else if (msg == "%CHUDAN") {
      sendBoth("#CHUDAN");
      return HandleResult::Terminate;
    } else if (msg.starts_with("+") || msg.starts_with("-")) {
      Color color = msg.starts_with("+") ? Color::Black : Color::White;
      if (game.next() == color) {
        auto ret = MoveFromCsaMove(msg, game.position);
        if (holds_alternative<Move>(ret)) {
          auto mv = get<Move>(ret);
          switch (game.apply(mv)) {
          case Game::ApplyResult::Ok:
            game.moves.push_back(mv);
            sendBoth(msg + "," + seconds());
            update();
            break;
          case Game::ApplyResult::Illegal:
            sendBoth("#ILLEGAAL_MOVE");
            sendWin(OpponentSource(d));
            return HandleResult::Terminate;
          case Game::ApplyResult::Repetition:
            sendBoth("#SENNICHITE");
            sendBoth("#DRAW");
            return HandleResult::Terminate;
          case Game::ApplyResult::CheckRepetitionBlack:
            sendBoth("#OUTE_SENNICHITE");
            sendWin(Color::White);
            return HandleResult::Terminate;
          case Game::ApplyResult::CheckRepetitionWhite:
            sendBoth("#OUTE_SENNICHITE");
            sendWin(Color::Black);
            return HandleResult::Terminate;
          }
        } else {
          sendBoth("#ILLEGAAL_MOVE");
          sendWin(OpponentSource(d));
          return HandleResult::Terminate;
        }
      } else {
        sendBoth("#ILLEGAL_ACTION");
        sendWin(OpponentSource(d));
        return HandleResult::Terminate;
      }
    } else if (msg == "%TORYO") {
      sendBoth("%TORYO," + seconds());
      sendBoth("#RESIGN");
      sendWin(OpponentSource(d));
      return HandleResult::Terminate;
    }
    return HandleResult::Continue;
  }

  void start() {
    sendBoth("START:" + gameId);
    update();
  }

  void sendGameSummary() {
    sendBoth("BEGIN Game_Summary");
    sendBoth("Protocol_Version:1.2");
    sendBoth("Protocol_Mode:Server");
    sendBoth("Format:Shogi 1.0");
    sendBoth("Game_ID:" + gameId);
    if (localColor == Color::Black) {
      sendBoth("Name+:" + local->name());
      sendBoth("Name-:" + remoteUsername);
      sendRemote("Your_Turn:-");
      sendLocal("Your_Turn:+");
    } else {
      sendBoth("Name+:" + remoteUsername);
      sendBoth("Name-:" + local->name());
      sendRemote("Your_Turn:+");
      sendLocal("Your_Turn:-");
    }
    if (game.handicap_ == Handicap::平手) {
      sendBoth("To_Move:+");
    } else {
      sendBoth("To_Move:-");
    }
    sendBoth("BEGIN Position");
    for (int y = 0; y < 9; y++) {
      string line = "P" + to_string(y + 1);
      for (int x = 0; x < 9; x++) {
        Piece p = game.position.pieces[x][y];
        Color color = ColorFromPiece(p);
        if (auto str = CsaStringFromPiece(p, IsPromotedPiece(p) ? 1 : 0); str) {
          line += (color == Color::Black ? "+" : "-") + *str;
        } else {
          line += " * ";
        }
      }
      sendBoth(line);
    }
    {
      string line = "P+";
      for (auto p : game.handBlack) {
        line += "00" + *CsaStringFromPiece(static_cast<PieceUnderlyingType>(p), 0);
      }
      sendBoth(line);
    }
    {
      string line = "P-";
      for (auto p : game.handWhite) {
        line += "00" + *CsaStringFromPiece(static_cast<PieceUnderlyingType>(p), 0);
      }
      sendBoth(line);
    }
    if (game.handicap_ == Handicap::平手) {
      sendBoth("+");
    } else {
      sendBoth("-");
    }
    sendBoth("END Position");
    sendBoth("END Game_Summary");
  }

  shared_ptr<CsaServer::Peer> const local;
  Color const localColor;
  int const remoteSocket;
  string const remoteUsername;
  string gameId;
  Game game;
  chrono::system_clock::time_point time;
  int agrees = 0;
};

struct RemotePeer {
  explicit RemotePeer(int socket) : socket(socket) {}
  void send(std::string const &line) {
    string m = line + "\x0a";
    ::send(socket, m.c_str(), m.size(), 0);
    cout << "csa_server:<<< " << line << endl;
  }
  std::shared_ptr<Match> match;
  int socket;
  string username;
  std::recursive_mutex mut;
};

} // namespace

struct CsaServer::Impl {
  explicit Impl(int port) : port(port) {
    stop = false;
    thread(std::bind(&Impl::run, this)).detach();
  }

  ~Impl() {
    stop = true;
    close(socket);
    if (auto remote = this->remote.lock(); remote) {
      remote->send("#CHUDAN");
      close(remote->socket);
    }
  }

  struct Writer : public CsaServer::Writer {
    ~Writer() override {}

    void send(string const &msg) override {
      if (auto m = match.lock(); m) {
        m->handle(msg, Match::Source::Local);
      }
    }

    weak_ptr<Match> match;
  };

  shared_ptr<Writer> setLocalPeer(shared_ptr<Peer> peer, Color localColor, Handicap h, bool hand) {
    if (auto remote = this->remote.lock(); remote) {
      lock_guard<recursive_mutex> lk(remote->mut);
      auto m = make_shared<Match>(peer, localColor, remote->socket, remote->username, h, hand);
      m->sendGameSummary();
      remote->match = m;
      auto writer = make_shared<Writer>();
      writer->match = m;
      return writer;
    } else {
      Local l;
      l.peer = peer;
      l.color = localColor;
      l.handicap = h;
      l.hand = hand;
      auto writer = make_shared<Writer>();
      l.writer = writer;
      local = l;
      return writer;
    }
  }

  void unsetLocalPeer() {
    local = nullopt;
  }

  void run() {
    int socket = ::socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
      close(socket);
      return;
    }
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_aton("0.0.0.0", &addr.sin_addr);

    if (::bind(socket, (sockaddr *)&addr, sizeof(addr)) < 0) {
      int code = errno;
      string desc;
#define CATCH(code) \
  case (code):      \
    desc = #code;   \
    break;
      switch (code) {
        CATCH(EACCES)
        CATCH(EADDRINUSE)
        CATCH(EADDRNOTAVAIL)
        CATCH(EAFNOSUPPORT)
        CATCH(EBADF)
        CATCH(EDESTADDRREQ)
        CATCH(EFAULT)
        CATCH(EINVAL)
        CATCH(ENOTSOCK)
        CATCH(EOPNOTSUPP)
      default:
        desc = "(unknown)";
        break;
      }
#undef CATCH
      cerr << "bind failed: code=" << code << ", desc=" << desc << endl;
      close(socket);
      return;
    }
    this->socket = socket;
    listen(this->socket, 1);
    while (!stop) {
      int socket = accept(this->socket, nil, nil);
      if (socket == -1) {
        break;
      }
      if (auto existing = this->remote.lock(); existing) {
        close(socket);
        continue;
      }
      auto remote = make_shared<RemotePeer>(socket);
      auto weak = weak_ptr<RemotePeer>(remote);
      thread(std::bind(&Impl::loop, this, remote)).detach();
      this->remote = weak;
    }
  }

  void loop(shared_ptr<RemotePeer> peer) {
    string buffer;

    while (!stop) {
      char tmp[64];
      ssize_t len = recv(peer->socket, tmp, sizeof(tmp), 0);
      if (len <= 0) {
        break;
      }
      copy_n(tmp, len, back_inserter(buffer));
      string::size_type offset = 0;
      while (offset < buffer.size()) {
        auto found = buffer.find('\xa', offset);
        if (found == string::npos) {
          break;
        }
        string line = buffer.substr(offset, found);
        if (line.empty()) {
          peer->send("");
          continue;
        }
        cout << "csa_server:>>> " << line << endl;
        lock_guard<recursive_mutex> lock(peer->mut);

        if (!peer->username.empty() && line == "LOGOUT") {
          peer->send("LOGOUT:completed");
          if (peer->match && peer->match->agrees > 1) {
            peer->match->sendLocal("#CHUDAN");
          }
          close(peer->socket);
          return;
        }

        auto match = peer->match;
        if (match) {
          switch (match->handle(line, Match::Source::Remote)) {
          case Match::HandleResult::Continue:
            break;
          case Match::HandleResult::Terminate:
            peer->match.reset();
            close(peer->socket);
            return;
          }
        } else {
          if (peer->username.empty() && line.starts_with("LOGIN ")) {
            auto n = line.find(' ', 6);
            if (n != string::npos) {
              auto name = line.substr(6, n - 6);
              peer->send("LOGIN:" + name + " OK");
              peer->username = name;
              if (local) {
                auto m = make_shared<Match>(local->peer, local->color, peer->socket, peer->username, local->handicap, local->hand);
                if (local->writer) {
                  local->writer->match = m;
                }
                m->sendGameSummary();
                peer->match = m;
                local = nullopt;
              }
            }
          }
        }
        offset = found + 1;
      }
      buffer = buffer.substr(offset);
    }
  }

  optional<int> getCurrentPort() const {
    if (socket == -1) {
      return nullopt;
    }
    return port;
  }

  bool isGameReady() const {
    auto r = remote.lock();
    if (!r) {
      return false;
    }
    lock_guard<recursive_mutex> lk(r->mut);
    if (!r->match) {
      return false;
    }
    return r->match->agrees > 1;
  }

  atomic_bool stop;
  int socket = -1;
  int const port;
  struct Local {
    shared_ptr<Peer> peer;
    Color color;
    Handicap handicap;
    bool hand;
    shared_ptr<Writer> writer;
  };
  optional<Local> local;
  weak_ptr<RemotePeer> remote;
};

CsaServer::CsaServer(int port) : impl(make_unique<Impl>(port)) {
}

CsaServer::~CsaServer() {}

shared_ptr<CsaServer::Writer> CsaServer::setLocalPeer(std::shared_ptr<Peer> local, Color color, Handicap h, bool hand) {
  return impl->setLocalPeer(local, color, h, hand);
}

void CsaServer::unsetLocalPeer() {
  impl->unsetLocalPeer();
}

optional<int> CsaServer::port() const {
  return impl->getCurrentPort();
}

bool CsaServer::isGameReady() const {
  return impl->isGameReady();
}

CsaServerWrapper CsaServerWrapper::Create(int port) {
  CsaServerWrapper w;
  w.server = make_shared<CsaServer>(port);
  return w;
}

} // namespace sci
