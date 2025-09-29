#include <iostream>
#include <map>
#include <limits>

using namespace std;

struct Stats {
    double min_activity;
    double max_activity;
    int count;
};

int main() {
    int N;
    cin >> N;
    map<int, Stats> crew_stats;
    for (int i = 0; i < N; i++) {
        int time_stamp, id;
        double vit_d, acat, anti_tg, neural_activity, mch;

        cin >> time_stamp >> id >> vit_d >> acat >> anti_tg >> neural_activity >> mch;

        if (crew_stats.find(id) == crew_stats.end()) {
            crew_stats[id] = {neural_activity, neural_activity, 1};
        } else {

            if (neural_activity < crew_stats[id].min_activity) {
                crew_stats[id].min_activity = neural_activity;
            }
            if (neural_activity > crew_stats[id].max_activity) {
                crew_stats[id].max_activity = neural_activity;
            }
            crew_stats[id].count++;
        }
    }

    int suspect_id = -1;
    double min_range = numeric_limits<double>::max();
    double suspect_max_activity = -1;

    for (const auto& entry : crew_stats) {
        int id = entry.first;
        const Stats& stats = entry.second;

        if (stats.count < 2) continue;

        double range = stats.max_activity - stats.min_activity;

        if (range < min_range) {
            min_range = range;
            suspect_id = id;
            suspect_max_activity = stats.max_activity;
        }

        else if (range == min_range) {
            if (stats.max_activity > suspect_max_activity) {
                suspect_id = id;
                suspect_max_activity = stats.max_activity;
            }
        }
    }

    cout << suspect_id << endl;

    return 0;
}
