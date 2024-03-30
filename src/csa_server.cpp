#include <shogi_camera/shogi_camera.hpp>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

using namespace std;

namespace sci {

struct RemotePeer : public CsaServer::Peer {
  explicit RemotePeer(int socket) : socket(socket) {}
  void onmessage(std::string const &line) override {}
  void send(std::string const &line) override {
    string m = line + "\x0a";
    ::send(socket, m.c_str(), m.size(), 0);
    cout << "csa_server:<<< " << line << endl;
  }
  string name() const override {
    return username;
  }
  void setName(string const &n) {
    username = n;
  }
  int socket;
  string username;
};

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

  void setLocalPeer(shared_ptr<Peer> local, Color localColor, Handicap h, bool hand) {
    Local_ l;
    l.peer = local;
    l.color = localColor;
    l.handicap = h;
    l.hand = hand;
    this->local_ = l;
    if (auto remote = this->remote.lock(); remote) {
      sendGameSummary(*local, *remote, localColor, h, hand);
    }
  }

  void unsetLocalPeer() {
    local_ = nullopt;
  }

  void sendGameSummary(Peer &local, Peer &remote, Color localColor, Handicap h, bool hand) {
    if (info) {
      return;
    }
    info = make_unique<Info>(h, hand);
    auto sendboth = [&](string const &msg) {
      local.onmessage(msg);
      remote.send(msg);
    };
    sendboth("BEGIN Game_Summary");
    sendboth("Protocol_Version:1.2");
    sendboth("Protocol_Mode:Server");
    sendboth("Format:Shogi 1.0");
    sendboth("Game_ID:" + info->gameId);
    if (localColor == Color::Black) {
      sendboth("Name+:" + local.name());
      sendboth("Name-:" + remote.name());
      remote.send("Your_Turn:-");
      local.onmessage("Your_Turn:+");
    } else {
      sendboth("Name+:" + remote.name());
      sendboth("Name-:" + local.name());
      remote.send("Your_Turn:+");
      local.onmessage("Your_Turn:-");
    }
    sendboth("To_Move:+");
    sendboth("BEGIN Position");
    Game g(h, hand);
    for (int y = 0; y < 9; y++) {
      string line = "P" + to_string(y + 1);
      for (int x = 0; x < 9; x++) {
        Piece p = g.position.pieces[x][y];
        Color color = ColorFromPiece(p);
        if (auto str = CsaStringFromPiece(p, IsPromotedPiece(p) ? 1 : 0); str) {
          line += (color == Color::Black ? "+" : "-") + *str;
        } else {
          line += " * ";
        }
      }
      sendboth(line);
    }
    {
      string line = "P+";
      for (auto p : g.handBlack) {
        line += "00" + *CsaStringFromPiece(static_cast<PieceUnderlyingType>(p), 0);
      }
      sendboth(line);
    }
    {
      string line = "P-";
      for (auto p : g.handWhite) {
        line += "00" + *CsaStringFromPiece(static_cast<PieceUnderlyingType>(p), 0);
      }
      sendboth(line);
    }
    if (h == Handicap::平手) {
      sendboth("+");
    } else {
      sendboth("-");
    }
    sendboth("END Position");
    sendboth("END Game_Summary");
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

  void send(string const &msg) {
    auto remote = this->remote.lock();
    if (!remote) {
      return;
    }
    lock_guard<recursive_mutex> lock(mut);
    if (!info) {
      return;
    }
    auto local = this->local_;
    if (!local) {
      return;
    }
    auto sendboth = [local, remote](string const &msg) {
      remote->send(msg);
      if (local) {
        local->peer->onmessage(msg);
      }
    };
    if (msg.starts_with("AGREE")) {
      info->agrees++;
      if (info->agrees > 1) {
        sendboth("START:" + info->gameId);
        info->update();
      }
    } else if (msg.starts_with("+")) {
      if (info->game.next() == Color::Black) {
        auto ret = MoveFromCsaMove(msg, info->game.position);
        if (holds_alternative<Move>(ret)) {
          auto mv = get<Move>(ret);
          switch (info->game.apply(mv)) {
          case Game::ApplyResult::Repetition:
            sendboth("#SENNICHITE");
            sendboth("#DRAW");
            info.reset();
            break;
          case Game::ApplyResult::Illegal:
            sendboth("#ILLEGAAL_MOVE");
            local->peer->onmessage("#LOSE");
            remote->send("#WIN");
            info.reset();
            break;
          case Game::ApplyResult::Ok:
            info->game.moves.push_back(mv);
            sendboth(msg + "," + info->seconds());
            info->update();
            break;
          case Game::ApplyResult::CheckRepetitionBlack:
            sendboth("#OUTE_SENNICHITE");
            if (local->color == Color::Black) {
              local->peer->onmessage("#LOSE");
              remote->send("#WIN");
            } else {
              local->peer->onmessage("#WIN");
              remote->send("#LOSE");
            }
            info.reset();
            break;
          case Game::ApplyResult::CheckRepetitionWhite:
            sendboth("#OUTE_SENNICHITE");
            if (local->color == Color::Black) {
              local->peer->onmessage("#WIN");
              remote->send("#LOSE");
            } else {
              local->peer->onmessage("#LOSE");
              remote->send("#WIN");
            }
            info.reset();
            break;
          }
        } else {
          sendboth("#ILLEGAAL_MOVE");
          local->peer->onmessage("#LOSE");
          remote->send("#WIN");
          info.reset();
        }
      } else {
        sendboth("#ILLEGAL_ACTION");
        if (local->color == Color::Black) {
          local->peer->onmessage("#LOSE");
          remote->send("#WIN");
        } else {
          local->peer->onmessage("#WIN");
          remote->send("#LOSE");
        }
        info.reset();
      }
    } else if (msg.starts_with("-")) {
      if (info->game.next() == Color::White) {
        auto ret = MoveFromCsaMove(msg, info->game.position);
        if (holds_alternative<Move>(ret)) {
          auto mv = get<Move>(ret);
          switch (info->game.apply(mv)) {
          case Game::ApplyResult::Ok:
            info->game.moves.push_back(mv);
            sendboth(msg + "," + info->seconds());
            info->update();
            break;
          case Game::ApplyResult::Illegal:
            sendboth("#ILLEGAAL_MOVE");
            local->peer->onmessage("#LOSE");
            remote->send("#WIN");
            info.reset();
            break;
          case Game::ApplyResult::Repetition:
            sendboth("#SENNICHITE");
            sendboth("#DRAW");
            info.reset();
            break;
          case Game::ApplyResult::CheckRepetitionBlack:
            sendboth("#OUTE_SENNICHITE");
            if (local->color == Color::Black) {
              local->peer->onmessage("#LOSE");
              remote->send("#WIN");
            } else {
              local->peer->onmessage("#WIN");
              remote->send("#LOSE");
            }
            info.reset();
            break;
          case Game::ApplyResult::CheckRepetitionWhite:
            sendboth("#OUTE_SENNICHITE");
            if (local->color == Color::Black) {
              local->peer->onmessage("#WIN");
              remote->send("#LOSE");
            } else {
              local->peer->onmessage("#LOSE");
              remote->send("#WIN");
            }
            info.reset();
            break;
          }
        } else {
          sendboth("#ILLEGAAL_MOVE");
          local->peer->onmessage("#LOSE");
          remote->send("#WIN");
          info.reset();
        }
      } else {
        sendboth("#ILLEGAL_ACTION");
        local->peer->onmessage("#LOSE");
        remote->send("#WIN");
        info.reset();
      }
    } else if (msg == "%TORYO") {
      sendboth("%TORYO," + info->seconds());
      sendboth("#RESIGN");
      remote->send("#WIN");
      local->peer->onmessage("#LOSE");
      info.reset();
    } else if (msg == "%CHUDAN") {
      sendboth("#CHUDAN");
      info.reset();
    }
  }

  void loop(shared_ptr<RemotePeer> peer) {
    auto send = [peer](string const &msg) {
      peer->send(msg);
    };
    auto sendboth = [this, peer](string const &msg) {
      peer->send(msg);
      if (local_) {
        local_->peer->onmessage(msg);
      }
    };

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
          send("");
          continue;
        }
        cout << "csa_server:>>> " << line << endl;
        lock_guard<recursive_mutex> lock(mut);
        if (peer->username.empty() && line.starts_with("LOGIN ")) {
          auto n = line.find(' ', 6);
          if (n != string::npos) {
            auto name = line.substr(6, n - 6);
            send("LOGIN:" + name + " OK");
            peer->setName(name);
            if (local_) {
              sendGameSummary(*local_->peer, *peer, local_->color, local_->handicap, local_->hand);
            }
          }
        } else if (!peer->username.empty() && line == "LOGOUT") {
          send("LOGOUT:completed");
          close(peer->socket);
          return;
        } else if (line == "AGREE" && info) {
          info->agrees++;
          if (info->agrees > 1) {
            sendboth("START:" + info->gameId);
            info->update();
          }
        } else if (info && line == "AGREE " + info->gameId) {
          info->agrees++;
          if (info->agrees > 1) {
            sendboth("START:" + info->gameId);
            info->update();
          }
        } else if (info && line == "REJECT") {
          if (local_) {
            local_->peer->onmessage("REJECT:" + info->gameId + " by " + peer->username);
          }
          info.reset();
        } else if (info && line == "REJECT " + info->gameId) {
          if (local_) {
            local_->peer->onmessage("REJECT:" + info->gameId + " by " + peer->username);
          }
          info.reset();
        } else if (line == "%CHUDAN") {
          sendboth("#CHUDAN");
          info.reset();
        } else if (line.starts_with("+") && info) {
          if (info->game.next() == Color::Black) {
            auto ret = MoveFromCsaMove(line, info->game.position);
            if (holds_alternative<Move>(ret)) {
              auto mv = get<Move>(ret);
              switch (info->game.apply(mv)) {
              case Game::ApplyResult::Ok:
                info->game.moves.push_back(mv);
                sendboth(line + "," + info->seconds());
                info->update();
                break;
              case Game::ApplyResult::Illegal:
                sendboth("#ILLEGAAL_MOVE");
                send("#LOSE");
                if (local_) {
                  local_->peer->onmessage("#WIN");
                }
                info.reset();
                break;
              case Game::ApplyResult::Repetition:
                sendboth("#SENNICHITE");
                sendboth("#DRAW");
                info.reset();
                break;
              case Game::ApplyResult::CheckRepetitionBlack:
                sendboth("#OUTE_SENNICHITE");
                if (local_) {
                  if (local_->color == Color::Black) {
                    local_->peer->onmessage("#LOSE");
                    send("#WIN");
                  } else {
                    local_->peer->onmessage("#WIN");
                    send("#LOSE");
                  }
                }
                info.reset();
                break;
              case Game::ApplyResult::CheckRepetitionWhite:
                sendboth("#OUTE_SENNICHITE");
                if (local_) {
                  if (local_->color == Color::Black) {
                    local_->peer->onmessage("#WIN");
                    send("#LOSE");
                  } else {
                    local_->peer->onmessage("#LOSE");
                    send("#WIN");
                  }
                }
                info.reset();
                break;
              }
            } else {
              sendboth("#ILLEGAAL_MOVE");
              send("#LOSE");
              if (local_) {
                local_->peer->onmessage("#WIN");
              }
              info.reset();
            }
          } else {
            sendboth("#ILLEGAL_ACTION");
            send("#LOSE");
            if (local_) {
              local_->peer->onmessage("#WIN");
            }
            info.reset();
          }
        } else if (line.starts_with("-") && info) {
          if (info->game.next() == Color::White) {
            auto ret = MoveFromCsaMove(line, info->game.position);
            if (holds_alternative<Move>(ret)) {
              auto mv = get<Move>(ret);
              switch (info->game.apply(mv)) {
              case Game::ApplyResult::Ok:
                info->game.moves.push_back(mv);
                sendboth(line + "," + info->seconds());
                info->update();
                break;
              case Game::ApplyResult::Illegal:
                sendboth("#ILLEGAAL_MOVE");
                send("#LOSE");
                if (local_) {
                  local_->peer->onmessage("#WIN");
                }
                info.reset();
                break;
              case Game::ApplyResult::Repetition:
                sendboth("#SENNICHITE");
                sendboth("#DRAW");
                info.reset();
                break;
              case Game::ApplyResult::CheckRepetitionBlack:
                sendboth("#OUTE_SENNICHITE");
                if (local_) {
                  if (local_->color == Color::Black) {
                    local_->peer->onmessage("#LOSE");
                    send("#WIN");
                  } else {
                    local_->peer->onmessage("#WIN");
                    send("#LOSE");
                  }
                }
                info.reset();
                break;
              case Game::ApplyResult::CheckRepetitionWhite:
                sendboth("#OUTE_SENNICHITE");
                if (local_) {
                  if (local_->color == Color::Black) {
                    local_->peer->onmessage("#WIN");
                    send("#LOSE");
                  } else {
                    local_->peer->onmessage("#LOSE");
                    send("#WIN");
                  }
                }
                info.reset();
                break;
              }
            } else {
              sendboth("#ILLEGAAL_MOVE");
              send("#LOSE");
              if (local_) {
                local_->peer->onmessage("#WIN");
              }
              info.reset();
            }
          } else {
            sendboth("#ILLEGAL_ACTION");
            send("#LOSE");
            if (local_) {
              local_->peer->onmessage("#WIN");
            }
            info.reset();
          }
        } else if (line == "%TORYO" && info) {
          sendboth("%TORYO," + info->seconds());
          sendboth("#RESIGN");
          send("#LOSE");
          if (local_) {
            local_->peer->onmessage("#WIN");
          }
          info.reset();
        }
        offset = found + 1;
      }
      buffer = buffer.substr(offset);
    }
  }

  void setLocal(std::shared_ptr<Peer> local, Color color, Handicap h, bool hand) {
    Local_ l;
    l.peer = local;
    l.color = color;
    l.handicap = h;
    l.hand = hand;
    this->local_ = l;
  }

  optional<int> getCurrentPort() const {
    if (socket == -1) {
      return nullopt;
    }
    return port;
  }

  bool isGameReady() const {
    if (!info) {
      return false;
    }
    return info->agrees > 1;
  }

  atomic_bool stop;
  int socket = -1;
  int const port;
  struct Local_ {
    shared_ptr<Peer> peer;
    Color color;
    Handicap handicap;
    bool hand;
  };
  optional<Local_> local_;
  weak_ptr<RemotePeer> remote;
  std::recursive_mutex mut;

  struct Info {
    Info(Handicap h, bool hand) : game(h, hand) {
      gameId = "shogicamera";
      time = chrono::system_clock::now();
    }

    string seconds() const {
      auto elapsed = chrono::duration_cast<chrono::duration<double>>(chrono::system_clock::now() - time).count();
      return "T" + to_string(std::max(0, (int)floor(elapsed)));
    }

    void update() {
      time = chrono::system_clock::now();
    }

    string gameId;
    Game game;
    chrono::system_clock::time_point time;
    int agrees = 0;
  };
  std::unique_ptr<Info> info;
};

CsaServer::CsaServer(int port) : impl(make_unique<Impl>(port)) {
}

CsaServer::~CsaServer() {}

void CsaServer::setLocalPeer(std::shared_ptr<Peer> local, Color color, Handicap h, bool hand) {
  impl->setLocalPeer(local, color, h, hand);
}

void CsaServer::unsetLocalPeer() {
  impl->unsetLocalPeer();
}

void CsaServer::send(string const &msg) {
  impl->send(msg);
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
