#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <vector>
#include <algorithm>

/* NEED PHASE 2 algorithm when we have full rack.  Maximize number of slots that
 * contain things in the deck, maximize rows. */

using namespace std;

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
    ret += min(longest, 5) * 10;
  }
  return ret;
}

int execit(char* path, int rpipe, int wpipe) {
  pid_t pid = fork();
  if(pid == 0) {
    char buf1[32];
    char buf2[32];
    char* args[4];
    sprintf(buf1, "%d", wpipe);
    sprintf(buf2, "%d", rpipe);
    args[0] = path;
    args[1] = buf1;
    args[2] = buf2;
    args[3] = NULL;
    execve(path, args, NULL);
    cerr << "failed to call client" << endl;
    exit(1);
  }
}

void write_rack(FILE* f, const vector<int>& V) {
  for(int i = 0; i < V.size(); i++) {
    if(i) fprintf(f, " ");
    fprintf(f, "%d", V[i]);
  }
}

int popv(vector<int>& V) {
  int res = V.back();
  V.resize(V.size() - 1);
  return res;
}

int main(int argc, char** argv) {
  srand(time(NULL));

  int in_pipe[2][2];
  int out_pipe[2][2];
  pipe(in_pipe[0]);
  pipe(in_pipe[1]);
  pipe(out_pipe[0]);
  pipe(out_pipe[1]);

  FILE* opipe[2];
  FILE* ipipe[2];
  opipe[0] = fdopen(out_pipe[0][1], "w");
  opipe[1] = fdopen(out_pipe[1][1], "w");
  ipipe[0] = fdopen(in_pipe[0][0], "r");
  ipipe[1] = fdopen(in_pipe[1][0], "r");

  execit(argv[1], out_pipe[0][0], in_pipe[0][1]);
  execit(argv[2], out_pipe[1][0], in_pipe[1][1]);

  int p1sum = 0;
  int p2sum = 0;
  for(int iter = 1; ; iter++) {
    vector<int> deck;
    for(int j = 0; j < 80; j++) deck.push_back(j);
    random_shuffle(deck.begin(), deck.end());

    vector<int> r1, r2;
    for(int j = 0; j < 20; j++) {
      r1.push_back(deck[80 - j - 1]);
      r2.push_back(deck[60 - j - 1]);
    }
    deck.resize(40);

    vector<int> discard;
    discard.push_back(popv(deck));
    
    fprintf(opipe[0], "START 0 0 %d 0\n", discard.back());
    fprintf(opipe[1], "START 0 0 %d 0\n", discard.back());

    int last_idx = -1;
    const char* last_move = "no_move";
    int t;
    for(t = 0; t < 75 && eval_rack(r1) < 150 && eval_rack(r2) < 150; t++) {
      for(int j = 0; j < 2; j++) {
        fprintf(opipe[j], "MOVE 0 ");
        write_rack(opipe[j], j?r2:r1);
        fprintf(opipe[j], " %d 0 %s %d\n", discard.back(), last_move, last_idx);
        fflush(opipe[j]);

        char resp[128];
        fscanf(ipipe[j], "%120s %d", resp, &last_idx);

        if(string(resp) == "request_deck") {
          int nv = popv(deck);
          fprintf(opipe[j], "EXCHANGE 0 0 ");
          write_rack(opipe[j], j?r2:r1);
          fprintf(opipe[j], " %d\n", nv);
          fflush(opipe[j]);

          if(deck.empty()) {
            deck.swap(discard);
            sort(deck.begin(), deck.end());
            deck.resize(unique(deck.begin(), deck.end()) - deck.begin());
            random_shuffle(deck.begin(), deck.end());
          }

          fscanf(ipipe[j], "%d", &last_idx);
          discard.push_back(nv);
          swap((j?r2:r1)[last_idx], discard.back());
          last_move = "take_deck";
        } else {
          swap((j?r2:r1)[last_idx], discard.back());
          last_move = "take_discard";
        }
        
      }
    }
    int e1 = eval_rack(r1);
    int e2 = eval_rack(r2);
    if(e1 == 150 && e2 > 100) e2 = 100;
    if(e2 == 150 && e1 > 100) e1 = 100;

    p1sum += e1;
    p2sum += e2;
    cout << "SCORE " << iter << ": [" << p1sum << ", " << p2sum << "] "
         << e1 << " to " << e2 << " in " << t << endl;
  }
}
