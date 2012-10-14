#include <iostream>
#include <vector>
#include <fstream>
#include <cstdlib>

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

using namespace std;
using namespace xmlrpc_c;

FILE* opipe;
FILE* ipipe;

struct PingMethod : public method {
public:
  void execute(paramList const& paramList, value* const retvalP) {
    *retvalP = value_string("pong");
  }
};

int iparam(const value& val) {
  return static_cast<int>((value_int)val);
}

string sparam(const value& val) {
  return static_cast<string>((value_string)val);
}

vector<value> vparam(const value& val) {
  return ((value_array)val).vectorValueValue();
}

vector<int> viparam(const value& val) {
  vector<value> v = vparam(val);
  vector<int> ret;
  for(int i = 0; i < v.size(); i++) ret.push_back(iparam(v[i]));
  return ret;
}

map<string, value> mpparam(const value& val) {
  return static_cast<map<string, value> >((value_struct)val);
}

struct StartGameMethod : public method {
public:
  void execute(const paramList& paramList, value* const retvalP) {
    map<string, value> params = paramList.getStruct(0);
    int game_id = iparam(params["game_id"]);
    int player_id = iparam(params["player_id"]);
    int initial_discard = iparam(params["initial_discard"]);
    int other_player_id = iparam(params["other_player_id"]);
    fprintf(opipe, "START %d %d %d %d\n", game_id, player_id,
            initial_discard - 1, other_player_id);
    fflush(opipe);

    *retvalP = value_string("");
  }
};

struct GetMoveMethod : public method {
public:
  void execute(paramList const& paramList, value* const retvalP) {
    map<string, value> params = paramList.getStruct(0);
    int game_id = iparam(params["game_id"]);
    fprintf(opipe, "MOVE %d", game_id);

    vector<int> rack = viparam(params["rack"]);
    for(int i = 0; i < rack.size(); i++) {
      fprintf(opipe, " %d", rack[i] - 1);
    }
    int discard = iparam(params["discard"]);
    int microsecs = iparam(params["remaining_microseconds"]);
    fprintf(opipe, " %d %d", discard - 1, microsecs);

    vector<value> moves = vparam(params["other_player_moves"]);
    if(!moves.empty()) {
      map<string, value> mp = mpparam(vparam(moves[0])[1]);
      fprintf(opipe, " %s ", sparam(mp["move"]).c_str());
      if(mp.find("idx") != mp.end()) {
        fprintf(opipe, "%d\n", iparam(mp["idx"]));
      } else {
        fprintf(opipe, "-1\n");
      }
    } else {
      fprintf(opipe, " no_move -1\n");
    }
    fflush(opipe);

    char buf[128];
    int idx;
    fscanf(ipipe, "%120s%d", buf, &idx);

    map<string, value> result;
    result["move"] = value_string(string(buf));
    if(string(buf) == "request_discard") {
      result["idx"] = value_int(idx);
    }
    *retvalP = value_struct(result);
  }
};

struct GetDeckExchangeMethod : public method {
public:
  void execute(paramList const& paramList, value* const retvalP) {
    map<string, value> params = paramList.getStruct(0);
    int game_id = iparam(params["game_id"]);
    fprintf(opipe, "EXCHANGE %d", game_id);

    int microsecs = iparam(params["remaining_microseconds"]);
    fprintf(opipe, " %d", microsecs);

    vector<int> rack = viparam(params["rack"]);
    for(int i = 0; i < rack.size(); i++) {
      fprintf(opipe, " %d", rack[i] - 1);
    }

    int card = iparam(params["card"]);
    fprintf(opipe, " %d\n", card - 1);
    fflush(opipe);

    int move;
    fscanf(ipipe, "%d", &move);
    *retvalP = value_int(move);
  }
};

struct MoveResultMethod : public method {
public:
  void execute(paramList const& paramList, value* const retvalP) {
    map<string, value> params = paramList.getStruct(0);
    int game_id = iparam(params["game_id"]);
    string move = sparam(params["move"]);

    if(move == "illegal") {
      cerr << "ILLEGAL MOVE DETECTED" << endl;
    }

    *retvalP = value_string("");
  }
};

struct GameResultMethod : public method {
public:
  void execute(paramList const& paramList, value* const retvalP) {
    map<string, value> params = paramList.getStruct(0);
    int game_id = iparam(params["game_id"]);
    int my_score = iparam(params["your_score"]);
    int their_score = iparam(params["other_score"]);
    string reason = sparam(params["reason"]);

    cerr << "GAME END: " << reason << endl;
    fprintf(opipe, "END %d %d %d\n", game_id, my_score, their_score);
    fflush(opipe);

    *retvalP = value_string("");
  }
};


int main(int argc, char** argv) {
  int in_pipe[2];
  int out_pipe[2];
  pipe(in_pipe);
  pipe(out_pipe);
  opipe = fdopen(out_pipe[1], "w");
  ipipe = fdopen(in_pipe[0], "r");

  pid_t pid = fork();
  if(pid == 0) {
    char buf1[32];
    char buf2[32];
    char* args[4];
    sprintf(buf1, "%d", in_pipe[1]);
    sprintf(buf2, "%d", out_pipe[0]);
    args[0] = argv[1];
    args[1] = buf1;
    args[2] = buf2;
    args[3] = NULL;
    execve(argv[1], args, NULL);
    cerr << "failed to call client" << endl;
    return 1;
  }

  int port = atoi(argv[2]);
  try {
    registry reg;

    methodPtr ping_meth(new PingMethod);
    methodPtr start_game_meth(new StartGameMethod);
    methodPtr get_move_meth(new GetMoveMethod);
    methodPtr get_deck_exchange_meth(new GetDeckExchangeMethod);
    methodPtr move_result_meth(new MoveResultMethod);
    methodPtr game_result_meth(new GameResultMethod);

    reg.addMethod("ping", ping_meth);
    reg.addMethod("start_game", start_game_meth);
    reg.addMethod("get_move", get_move_meth);
    reg.addMethod("get_deck_exchange", get_deck_exchange_meth);
    reg.addMethod("move_result", move_result_meth);
    reg.addMethod("game_result", game_result_meth);

    serverAbyss srv(serverAbyss::constrOpt().registryP(&reg).
                    portNumber(port).logFileName("rpc.log"));
    srv.run();
    exit(1);
  } catch(const exception& e) {
    exit(2);
  }
  return 0;
}
