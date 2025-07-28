// main.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <cmath>
#include <cstring>

using namespace std;

// BigInt (string-based) operations
class BigInt {
public:
    string value;

    BigInt() { value = "0"; }
    BigInt(string v) { value = trim(v); }
    BigInt(const char* v) { value = trim(string(v)); }
    BigInt(long long v) { value = to_string(v); }

    static string trim(string s) {
        int i = 0;
        while (i < s.size() - 1 && s[i] == '0') i++;
        return s.substr(i);
    }

    static string add(string a, string b) {
        string res = "";
        int carry = 0, i = a.size() - 1, j = b.size() - 1;
        while (i >= 0 || j >= 0 || carry) {
            int x = i >= 0 ? a[i--] - '0' : 0;
            int y = j >= 0 ? b[j--] - '0' : 0;
            int sum = x + y + carry;
            carry = sum / 10;
            res = char(sum % 10 + '0') + res;
        }
        return trim(res);
    }

    static string multiply(string a, string b) {
        int m = a.size(), n = b.size();
        vector<int> prod(m + n, 0);
        for (int i = m - 1; i >= 0; i--)
            for (int j = n - 1; j >= 0; j--)
                prod[i + j + 1] += (a[i] - '0') * (b[j] - '0');

        for (int i = m + n - 1; i > 0; i--) {
            prod[i - 1] += prod[i] / 10;
            prod[i] %= 10;
        }

        string res = "";
        int i = 0;
        while (i < prod.size() && prod[i] == 0) i++;
        for (; i < prod.size(); i++) res += (prod[i] + '0');
        return res == "" ? "0" : res;
    }

    BigInt operator+(const BigInt& b) const { return BigInt(add(value, b.value)); }
    BigInt operator*(const BigInt& b) const { return BigInt(multiply(value, b.value)); }
    bool operator==(const BigInt& b) const { return value == b.value; }
    void print() const { cout << value << endl; }
};

// Decode value with given base to base 10
BigInt decodeValue(string val, int base) {
    BigInt res("0");
    BigInt powB("1");
    for (int i = val.length() - 1; i >= 0; i--) {
        int digit = isdigit(val[i]) ? val[i] - '0' : (tolower(val[i]) - 'a' + 10);
        BigInt term = powB * BigInt(to_string(digit));
        res = res + term;
        powB = powB * BigInt(to_string(base));
    }
    return res;
}

// Lagrange Interpolation at x = 0
BigInt lagrangeAtZero(const vector<pair<int, BigInt>>& points) {
    BigInt result("0");
    int k = points.size();
    for (int i = 0; i < k; i++) {
        int xi = points[i].first;
        BigInt yi = points[i].second;
        BigInt num("1");
        BigInt den("1");
        for (int j = 0; j < k; j++) {
            if (i == j) continue;
            int xj = points[j].first;
            num = num * BigInt(to_string(-xj));
            den = den * BigInt(to_string(xi - xj));
        }
        result = result + (yi * num); // simplified without division
    }
    return result;
}

map<int, pair<int, string>> parseJSON(const string& filename, int& n, int& k) {
    ifstream file(filename);
    string line, json;
    while (getline(file, line)) json += line;

    map<int, pair<int, string>> data;
    size_t pos = json.find("\"n\":");
    n = stoi(json.substr(pos + 5));
    pos = json.find("\"k\":");
    k = stoi(json.substr(pos + 5));

    for (int i = 1; i <= n || i <= 10; i++) {
        string idx = to_string(i);
        size_t p = json.find("\"" + idx + "\":");
        if (p == string::npos) continue;
        size_t baseStart = json.find("\"base\": \"", p) + 9;
        size_t baseEnd = json.find("\"", baseStart);
        int base = stoi(json.substr(baseStart, baseEnd - baseStart));
        size_t valStart = json.find("\"value\": \"", p) + 10;
        size_t valEnd = json.find("\"", valStart);
        string val = json.substr(valStart, valEnd - valStart);
        data[i] = {base, val};
    }
    return data;
}

// Generate all k-sized combinations
void generateCombinations(const vector<pair<int, BigInt>>& allPoints, int k, int start, vector<pair<int, BigInt>>& current, vector<vector<pair<int, BigInt>>>& result) {
    if (current.size() == k) {
        result.push_back(current);
        return;
    }
    for (int i = start; i < allPoints.size(); i++) {
        current.push_back(allPoints[i]);
        generateCombinations(allPoints, k, i + 1, current, result);
        current.pop_back();
    }
}

void solve(const string& filename) {
    int n, k;
    map<int, pair<int, string>> rawData = parseJSON(filename, n, k);

    vector<pair<int, BigInt>> allPoints;
    for (auto it = rawData.begin(); it != rawData.end(); ++it) {
        int x = it->first;
        int base = it->second.first;
        string val = it->second.second;
        BigInt y = decodeValue(val, base);
        allPoints.push_back(make_pair(x, y));
    }

    map<string, int> freq;
    BigInt bestSecret;
    int maxFreq = 0;
    vector<vector<pair<int, BigInt>>> combinations;
    vector<pair<int, BigInt>> temp;
    generateCombinations(allPoints, k, 0, temp, combinations);

    for (int i = 0; i < combinations.size(); ++i) {
        BigInt secret = lagrangeAtZero(combinations[i]);
        freq[secret.value]++;
        if (freq[secret.value] > maxFreq) {
            maxFreq = freq[secret.value];
            bestSecret = secret;
        }
    }

    cout << "Secret from " << filename << ": ";
    bestSecret.print();
    cout << "Likely faulty shares (if any):\n";

    for (int i = 0; i < allPoints.size(); ++i) {
        int x = allPoints[i].first;
        vector<pair<int, BigInt>> trial;
        for (int j = 0; j < allPoints.size(); ++j) {
            if (allPoints[j].first != x) {
                trial.push_back(allPoints[j]);
            }
            if (trial.size() == k) break;
        }
        BigInt trySecret = lagrangeAtZero(trial);
        if (!(trySecret == bestSecret)) {
            cout << "Share (" << x << ") may be faulty." << endl;
        }
    }
}

int main() {
    solve("testcase1.json");
    solve("testcase2.json");
    return 0;
}