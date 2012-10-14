#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>

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
    decksz = 39;
    shuffled = false;
  }

  vector<int> racka;
  vector<int> rackb;
  int discard;
  vector<bool> deck;
  vector<bool> out;
  int decksz;
  bool shuffled;
};

void shuffle_deck(GameState& st) {
  if(st.decksz == 0) {
    st.deck = st.out;
    st.out = vector<bool>(80, false);
    st.decksz = 39;
    st.shuffled = true;
    //cout << "SHUFFLE SIZE: " << count(st.deck.begin(), st.deck.end(), true) << endl;
  }
}

bool move_disabled(GameState& st, int ind) {
  if(!st.shuffled) return false;
  set<int> used(st.racka.begin(), st.racka.end());
  for(int i = 0; i < 80; i++) {
    if(st.deck[i]) used.insert(i);
    if(st.out[i]) used.insert(i);
  }
  vector<int> r2;
  for(int i = 0; i < 80; i++) {
    if(used.find(i) == used.end()) {
      r2.push_back(i);
    }
  }
  for(int i = 0; i < r2.size(); ) {
    int sz = 1;
    while(i + sz < r2.size() && r2[i] + sz == r2[i + sz]) {
      sz++;
    }
    if(sz >= 4) {
      if(st.racka[ind] + 1 == r2[i] ||
         st.racka[ind] - 1 == r2[i + sz - 1]) {
        return true;
      }
    }
    i += sz;
  }
  return false;
}

int eval_rack(const vector<int>& A) {
  int ret = 5;
  for(int i = 1; i < 20; i++) {
    if(A[i] < A[i - 1]) {
      break;
    }
    ret += 5;
  }
  if(ret == 100) {
    int longest = 0;
    int num = 0;
    for(int i = 0; i < A.size(); ) {
      int sz = 1;
      while(i + sz < A.size() && A[i] + sz == A[i + sz]) {
        sz++;
      }
      if(sz > longest) {
        longest = sz;
        num = 1;
      } else if(sz == longest) {
        num++;
      }
      i += sz;
    }
    ret += min(longest, 5) * 10; // + num;
  }
  return ret;
}

vector<int> relabel;
vector<int> delabel;

pair<int, vector<int> > restricted(const GameState& st) {
  int lst = 0;
  relabel = vector<int>(80, -1);
  delabel = vector<int>(80, -1);

  set<int> r2st;
  for(int i = 0; i < 20; i++) r2st.insert(st.rackb[i]);
  for(int i = 0; i < 80; i++) {
    if((st.decksz < 2 || !st.out[i]) && r2st.find(i) == r2st.end()) {
      delabel[lst] = i;
      relabel[i] = lst++;
    }
  }
  vector<int> ret;
  for(int i = 0; i < st.racka.size(); i++) {
    ret.push_back(relabel[st.racka[i]]);
  }
  return make_pair(lst, ret);
}

double DP[30][30];

double simple_expect(int slots, int range) {
  if(range < slots) return 1e300;
  return exp(44 - 2.9 * range / slots) * pow(slots, 4);
}

double _valuate(const vector<int>& A, int loi, int hii, int lov, int hiv) {
  if(loi == hii) return 0;
  if(hii - loi > hiv - lov) return 1e300;

  double& ref = DP[loi][hii];
  if(ref != -1) return ref;

  ref = exp(0.06 * -(loi + hii)) * simple_expect(hii - loi, hiv - lov);
  for(int i = loi; i < hii; ) {
    if(A[i] < lov || hiv <= A[i]) {
      i++;
      continue;
    }

    int sz = 1;
    while(i + sz < A.size() && delabel[A[i]] + sz == delabel[A[i + sz]]) sz++;

    ref = min(ref, (_valuate(A, loi, i, lov, A[i]) +
                    _valuate(A, i + 1, hii, A[i] + 1, hiv)) /
                    pow(min(5, sz), 2));
    i += sz;
  }

  return ref;
}

double valuate(const GameState& st) {
  pair<int, vector<int> > rres = restricted(st);
  for(int i = 0; i < 30; i++) for(int j = 0; j < 30; j++) DP[i][j] = -1;
  return _valuate(rres.second, 0, rres.second.size(), 0, rres.first);
}

double phase2(const GameState& st) {
  const vector<int>& A = st.racka;

  double result = 0;
  for(int i = 0; i < A.size(); ) {
    int sz = 1;
    while(i + sz < A.size() && A[i] + sz == A[i + sz]) {
      sz++;
    }
    
    result += exp(sz);
    if(A[i] > 0 && (st.deck[A[i] - 1] || st.out[A[i] - 1])) {
      result += exp(sz) / 23.0;
    }
    if(A[i + sz - 1] < 79 &&
       (st.deck[A[i + sz - 1] + 1] | st.out[A[i + sz - 1] + 1])) {
      result += exp(sz) / 23.0;
    }
    i += sz;
  }
  return result;
}

bool is_sorted(const vector<int>& V) {
  for(int i = 1; i < V.size(); i++) {
    if(V[i] < V[i - 1]) return false;
  }
  return true;
}

bool is_close_to_winning(const GameState& st) {
  if(!st.shuffled) return false;
  set<int> used(st.racka.begin(), st.racka.end());
  for(int i = 0; i < 80; i++) {
    if(st.deck[i]) used.insert(i);
    if(st.out[i]) used.insert(i);
  }
  vector<int> r2;
  for(int i = 0; i < 80; i++) {
    if(used.find(i) == used.end()) {
      r2.push_back(i);
    }
  }
  for(int i = 0; i < r2.size(); ) {
    int sz = 1;
    while(i + sz < r2.size() && r2[i] + sz == r2[i + sz]) {
      sz++;
    }
    if(sz >= 4) {
      return true;
    }
    i += sz;
  }
  return false;
}

double eval(const GameState& st, int turns) {
  int ret = eval_rack(st.racka);

  double val = -ret;
  if(ret < 100) {
    memset(DP, 0, sizeof(DP));
    val += 1e-0 * (1.0 - exp(0.7 * -turns)) * valuate(st);
//cout << "PHASE1 " << ret << ", " << val << ", " << is_sorted(st.racka) << endl;
  } else {
//cout << "PHASE2 " << ret << ", " << val << endl;
    val = -100 - phase2(st);
  }
  return val;
}

pair<int, double> werk(GameState& st, int card, int turns,
                       bool override = false) {
  vector<int>& A = st.racka;

  double ebest = 1e300; //eval(st, turns);
  int pos = -1;
  for(int i = 0; i < A.size(); i++) {
    if(!override && move_disabled(st, i)) continue;

    swap(A[i], card);
    double res = eval(st, turns);
    if(res < ebest) {
      ebest = res;
      pos = i;
    }
    swap(A[i], card);
  }
  if(!override && is_sorted(A) && -100 < ebest) {
    return werk(st, card, turns, true);
  }
  if(pos == -1) {
    cout << "COULD NOT FIND A PLACE TO MOVE" << endl;
    pos = 0;
  }
  return make_pair(pos, ebest);
}

int place_card(GameState& st, int card, int turns) {
  return werk(st, card, turns).first;
}

double exp_turns(GameState& st, int card, int turns) {
  return werk(st, card, turns).second;
}

int main(int argc, char** argv) {
/*
  GameState st;
  st.racka.resize(0);
  st.racka.push_back(1);
  st.racka.push_back(72);
  st.racka.push_back(78);
  st.racka.push_back(20);
  st.racka.push_back(50);
  st.racka.push_back(40);
  cout << valuate(st) << endl;
  return 0;
*/

  if(argc != 3) {
    cerr << "BAD CALL" << endl;
    return 1;
  }

  opipe = fdopen(atoi(argv[1]), "w");
  ipipe = fdopen(atoi(argv[2]), "r");

  bool last_sorted = false;
  int turns_left = -1;
  GameState state;
  while(1) {
    string cmd = get_string();
    if(cmd == "START") {
      last_sorted = false;
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

      if(last_sorted && !is_sorted(state.racka)) {
        cout << "Negative sorting edge?" << endl;
      } else if(is_sorted(state.racka)) {
        last_sorted = true;
      }

      /* Clear out everything we know not to be in the deck anymore. */
      state.deck[odiscard] = state.deck[state.discard] = false;
      for(int i = 0; i < 20; i++) {
        state.deck[state.racka[i]] = false;
        if(state.rackb[i] != -1) state.deck[state.rackb[i]] = false;
      }

      double etake = exp_turns(state, state.discard, turns_left);
      double edraw = 0;
      int draw_total = 0;
      for(int i = 0; i < 80; i++) {
        if(state.deck[i]) {
          edraw += exp_turns(state, i, turns_left);
          draw_total++;
        }
      }
      edraw /= draw_total;

      if(etake < edraw) {
        int ind = place_card(state, state.discard, turns_left);
        state.discard = state.racka[ind];
        fprintf(opipe, "request_discard %d\n", ind);
      } else {
        state.out[state.discard] = true;
        fprintf(opipe, "request_deck -1\n");
      }
      fflush(opipe);
    } else if(cmd == "EXCHANGE") {
      int micros;
      fscanf(ipipe, "%d%d", &game_id, &micros);
      read_rack();
      int card;
      fscanf(ipipe, "%d", &card);

      state.deck[card] = false;
      state.decksz--;
      shuffle_deck(state);

      int ind = place_card(state, card, turns_left);
      state.discard = state.racka[ind];
      fprintf(opipe, "%d\n", ind);
      fflush(opipe);
    } else if(cmd == "END") {
      int my_score, their_score;
      fscanf(ipipe, "%d%d%d", &game_id, &my_score, &their_score);
      cerr << "Game over: " << my_score << ", " << their_score << 
              " (" << turns_left << " turns left)" << endl;
    } else {
      cerr << "UNKNOWN COMMAND" << endl;
      return 1;
    }
  }
  return 0;
}
