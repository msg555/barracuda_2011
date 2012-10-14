#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <cstring>

using namespace std;

FILE* opipe;
FILE* ipipe;

string get_string() {
  char buf[128];
  fscanf(ipipe, "%120s", buf);
  return string(buf);
}

vector<int> read_rack() {
  vector<int> ret;
  for(int i = 0; i < 20; i++) {
    int x;
    fscanf(ipipe, "%d", &x);
    ret.push_back(x);
  }
  return ret;
}

int game_id;
int player_id;
int other_player_id;

struct GameState {
  GameState() {
    racka = rackb = vector<int>(20, -1);
    discard = -1;
    deck = vector<bool>(80, true);
    out = vector<bool>(80, false);
    decksz = 40;
  }

  vector<int> racka;
  vector<int> rackb;
  int discard;
  vector<bool> deck;
  vector<bool> out;
  int decksz;
};

void shuffle_deck(GameState& st) {
  if(st.decksz == 0) {
    st.deck = st.out;
    st.out = vector<bool>();
    st.decksz = count(st.deck.begin(), st.deck.end(), true);
  }
}

int main(int argc, char** argv) {
  if(argc != 3) {
    cerr << "BAD CALL" << endl;
    return 1;
  }

  opipe = fdopen(atoi(argv[1]), "w");
  ipipe = fdopen(atoi(argv[2]), "r");

  int turns_left = -1;
  GameState state;
  while(1) {
    string cmd = get_string();
    if(cmd == "START") {
      state = GameState();
      fscanf(ipipe, "%d%d%d%d", &game_id, &player_id,
             &state.discard, &other_player_id);
      turns_left = 75;
    } else if(cmd == "MOVE") {
      --turns_left;
      fscanf(ipipe, "%d", &game_id);
      state.racka = read_rack();
      int odiscard = state.discard;
      int micros;
      fscanf(ipipe, "%d%d", &state.discard, &micros);

      string move = get_string();
      int idx; fscanf(ipipe, "%d", &idx);
      if(move == "take_discard") {
        state.rackb[idx] = odiscard;
      } else if(move == "take_deck") {
        state.decksz--;
        state.rackb[idx] = -1;
        state.out[odiscard] = true;
        shuffle_deck(state);
      } else if(move == "no_move" || move == "illegal" ||
                move == "timed_out") {
        /* Nothing to do... */
      } else {
        cerr << "UNKNOWN MOVE" << endl;
        return 1;
      }

      /* Clear out everything we know not to be in the deck anymore. */
      state.deck[odiscard] = state.deck[state.discard] = false;
      for(int i = 0; i < 20; i++) {
        state.deck[state.racka[i]] = false;
        if(state.rackb[i] != -1) state.deck[state.rackb[i]] = false;
      }

      state.out[state.discard] = true;
      fprintf(opipe, "request_deck -1\n");
      //fprintf(opipe, "request_discard 0\n");
      fflush(opipe);
    } else if(cmd == "EXCHANGE") {
      int micros;
      fscanf(ipipe, "%d%d", &game_id, &micros);
      read_rack();
      int card;
      fscanf(ipipe, "%d", &card);

      int ind = max_element(state.racka.begin(), state.racka.end()) -
                            state.racka.begin();
      fprintf(opipe, "%d\n", ind);
      fflush(opipe);
    } else if(cmd == "END") {
      int my_score, their_score;
      fscanf(ipipe, "%d%d%d", &game_id, &my_score, &their_score);
      cerr << "Game over: " << my_score << ", " << their_score << endl;
    } else {
      cerr << "UNKNOWN COMMAND" << endl;
      return 1;
    }
  }
  return 0;
}
