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
  void send(std::string const &line) override {}
  int socket;
};

struct CsaServer::Impl {
  Impl(int port, shared_ptr<Peer> local, Color color) : port(port), local(local), color(color) {
    stop = false;
    thread(std::bind(&Impl::run, this)).detach();
  }

  ~Impl() {
    stop = true;
    close(socket);
    if (auto remote = this->remote.lock(); remote) {
      close(remote->socket);
    }
  }

  void run() {
    this->socket = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_aton("0.0.0.0", &addr.sin_addr);

    if (::bind(this->socket, (sockaddr *)&addr, sizeof(addr)) < 0) {
      cerr << "bind failed" << endl;
      return;
    }
    listen(this->socket, 1);
    while (!stop) {
      int socket = accept(this->socket, nil, nil);
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
    optional<string> username;
    auto send = [peer](string const &msg) {
      string m = msg + "\x0a";
      ::send(peer->socket, m.c_str(), m.size(), 0);
      cout << "csa_server:<<< " << msg << endl;
    };
    while (!stop) {
      char tmp[64];
      ssize_t len = recv(peer->socket, tmp, sizeof(tmp), 0);
      if (len == 0) {
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
        cout << "csa_server:>>> " << line << endl;
        if (!username && line.starts_with("LOGIN ")) {
          auto n = line.find(' ', 6);
          if (n != string::npos) {
            auto name = line.substr(6, n - 6);
            send("LOGIN:" + name + " OK");
            username = name;
          }
        } else if (username && line == "LOGOUT") {
          send("LOGOUT:completed");
          close(peer->socket);
          return;
        }
        offset = found + 1;
      }
      buffer = buffer.substr(offset);
    }
  }

  atomic_bool stop;
  int socket;
  int port;
  shared_ptr<Peer> local;
  weak_ptr<RemotePeer> remote;
  Color color;
};

CsaServer::CsaServer(int port, shared_ptr<CsaServer::Peer> local, Color color) : impl(make_unique<Impl>(port, local, color)) {
}

CsaServer::~CsaServer() {}

} // namespace sci
