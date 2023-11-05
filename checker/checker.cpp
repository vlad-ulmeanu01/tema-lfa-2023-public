#include "checker_utils.h"

const int MAX_ACCEPTED_LEN = 200'000'000;
const int TIME_LIMIT_SECONDS = 120;
const int MAX_TIME_LIMIT_SECONDS = 800;

///daca raspunsul participantului este acceptat de DFA, are lungimea minim k si are lungimea maxim 2e8, intorc (|w| lui, k).
///daca raspunsul participantului este -1, intorc (-1, k).
std::optional<std::pair<int, int>> evaluateOutput(std::ifstream &fin, std::ifstream &participant) {
#define CHECK_READ_PAIR(x) if (!(x).second.empty()) { std::cout << (x).second << '\n'; return std::nullopt; }
#define BYE(bye_str) { std::cout << bye_str << '\n'; return std::nullopt; }

    auto t = readInt(fin, 1, 3, "t"); CHECK_READ_PAIR(t);
    auto n = readInt(fin, 1, 1'000'000, "n"); CHECK_READ_PAIR(n);
    auto sigma = readInt(fin, 1, 26, "|Sigma|"); CHECK_READ_PAIR(sigma);
    auto m = readInt(fin, 1, n.first, "m"); CHECK_READ_PAIR(m);
    auto k = readInt(fin, 1, MAX_ACCEPTED_LEN, "k"); CHECK_READ_PAIR(k);
    auto q_init = readInt(fin, 1, n.first, "q_init"); CHECK_READ_PAIR(q_init);
    q_init.first--;

    std::vector<int> q_dest(m.first);
    for (int i = 0; i < m.first; i++) {
        auto x = readInt(fin, 1, n.first, "q_dest"); CHECK_READ_PAIR(x);
        q_dest[i] = x.first - 1;
    }

    std::sort(q_dest.begin(), q_dest.end());

    std::vector<std::vector<int>> g(n.first, std::vector<int>(sigma.first));
    for (int i = 0; i < n.first; i++) {
        for (int j = 0; j < sigma.first; j++) {
            auto x = readInt(fin, 1, n.first, "node"); CHECK_READ_PAIR(x);
            g[i][j] = x.first - 1;
        }
    }

    auto partWref = readInt(participant, -1, MAX_ACCEPTED_LEN, "|w_ref| participant"); CHECK_READ_PAIR(partWref);

    if (partWref.first == -1) {
        return std::optional<std::pair<int, int>>(std::make_pair(partWref.first, k.first));
    }

    if (partWref.first < k.first) {
        BYE("|w_ref| participant is not in the interval [" + std::to_string(k.first) + ", " + std::to_string(MAX_ACCEPTED_LEN) + "].");
    }

    ///TODO ceva ca fgets.
    std::string s; participant >> s;
    if ((int)s.size() != partWref.first) BYE("participant's string's length is different from its stated length.");

    int now = q_init.first;
    for (char ch: s) {
        if (ch < 'a' || ch - 'a' >= sigma.first) BYE("character that isn't in Sigma found in participant's word.");
        now = g[now][ch - 'a'];
    }

    if (!std::binary_search(q_dest.begin(), q_dest.end(), now)) BYE("participant's word not accepted by D.");

#undef BYE
#undef CHECK_READ_PAIR

    return std::optional<std::pair<int, int>>(std::make_pair(partWref.first, k.first));
}

int main(int argc, char **argv) {
    ///maresc dimensiunea stivei la 256MB.
    struct rlimit rl;
    const rlim_t kStackSize = 256L * 1024L * 1024L;
    assert(!getrlimit(RLIMIT_STACK, &rl));
    rl.rlim_cur = kStackSize;
    assert(!setrlimit(RLIMIT_STACK, &rl));

    if (argc != 3) {
        std::cerr << "usage: ./checker <..locatie checker_info.txt..> <..director makefile cod de testat..>" << '\n';
        std::cerr << "ex ./checker checker_info.txt ../implementare_uva/" << '\n';
        return 0;
    }

    std::string implLoc(argv[2]);
    assert(implLoc.back() == '/');

    system((std::string("make --no-print-directory -C ") + implLoc + " build").c_str()); ///compilez codul participantului.

    std::ifstream checkerInfo(argv[1]);
    assert(checkerInfo.is_open());

    double maxTimeLeft = MAX_TIME_LIMIT_SECONDS;
    double scor = 0;
    std::string checkerLine;

    std::cout << std::fixed << std::setprecision(4);
    while (std::getline(checkerInfo, checkerLine)) {
        if (!checkerLine.empty() && checkerLine[0] != '#') {
            ///TODO fara boost?
            boost::tokenizer<boost::char_separator<char>> tokenizer(checkerLine, boost::char_separator<char>(" \n"));

            std::vector<std::string> tokens;
            for (const std::string &tok: tokenizer) {
                tokens.push_back(tok);
            }

            std::string &testInput = tokens[0];
            int testType = atoi(tokens[1].c_str());
            int scorNow = atoi(tokens[2].c_str());
            int lenWRef = atoi(tokens[3].c_str());

            ///copiez inputul testului in input.txt.
            system((std::string("cp ") + testInput + " " + implLoc + "input.txt").c_str());

            std::cout << "running test " << testInput << ":\n";

            ///rulez codul participantului.
            int timeLimitParticipant = std::max(1, std::min(TIME_LIMIT_SECONDS, (int)maxTimeLeft));

            auto start = std::chrono::steady_clock::now();
            int retVal = system((std::string("timeout ") +
                                std::to_string(timeLimitParticipant) +
                                std::string(" make --no-print-directory -C ") + implLoc + " run").c_str());

            double timePassed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count() / 1e6;
            maxTimeLeft -= timePassed;

            if (retVal == 124) {
                std::cout << "(" << testInput << ") time limit of " << TIME_LIMIT_SECONDS << " s exceeded.\n";
            } else if (retVal != 0) {
                std::cout << "(" << testInput << ") participant's process ended unexpectedly with retVal = " << retVal << '\n';
                std::cout << "(" << testInput << ") process ran for: " << timePassed << " s.\n";
            } else {
                std::cout << "(" << testInput << ") process ran for: " << timePassed << " s.\n";

                std::ifstream fin(testInput);
                std::ifstream participant(implLoc + "output.txt");

                std::optional<std::pair<int, int>> evalAns = evaluateOutput(fin, participant);
                double testScore = 0;

                if (evalAns.has_value()) {
                    int partWref, k;
                    std::tie(partWref, k) = evalAns.value();

                    if (partWref == -1) {
                        if (lenWRef == -1) testScore = scorNow;
                    } else if (lenWRef == -1) {
                        testScore = 0;
                    } else if(testType == 1) {
                        if (partWref > lenWRef) testScore = (double)scorNow * 0.5;
                        else testScore = scorNow;
                    } else if (testType == 2) {
                        if (partWref == lenWRef) testScore = scorNow;
                    } else {
                        int A = lenWRef - k + 1, B = partWref - k + 1;
                        double raport = 2 * (double)A / (A + B);
                        testScore = scorNow * raport;

                        std::cout << "(subtask 3) test multiplier = " << raport << '\n';
                    }
                }

                std::cout << "(" << testInput << ") current test score: " << testScore << '\n';
                scor += testScore;

                fin.close();
                participant.close();
            }

            ///sterg input.txt, output.txt
            system((std::string("rm ") + implLoc + "input.txt").c_str());
            system((std::string("rm ") + implLoc + "output.txt").c_str());

            std::cout << "score after " << testInput << ": " << scor << '\n';
            std::cout << "time left till timeout: " << maxTimeLeft << '\n';
        }
    }

    ///sterg executabilul participantului.
    system((std::string("make --no-print-directory -C ") + implLoc + " clean").c_str());

    checkerInfo.close();

    std::cout << "total score: " << scor << '\n';

    return 0;
}
