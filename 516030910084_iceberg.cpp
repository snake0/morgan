#include<fstream>
#include<iostream>
#include<sstream>
#include<string>
#include<vector>

using namespace std;


/*efficient random number generator*/
static unsigned long x = 123456789, y = 362436069, z = 521288629;
unsigned long xorshf96(void) {
	unsigned long t;
	x ^= x << 16;
	x ^= x >> 5;
	x ^= x << 1;
	t = x; x = y; y = z;
	z = t ^ x ^ y;
	return z % 4;
}
/*change separators to be blanks*/
inline void blank(string& line) {
	for (char &k : line)
		if (k == ',' || k == ':')
			k = ' ';
}


/*use class time to represent the time of the deal*/
struct time {
	int h = 9, m = 0;
	time(int hour) :h(hour) {}
	time() = default;
};
inline int mark(time& t) {
	return 60 * (t.h - 9) + t.m;
}
time operator+(time& t, int k) {
	t.m += k;
	if (t.m >= 60) {
		t.m -= 60;
		t.h += 1;
	}
	return t;
}

/*use class line to represent a line in the csv file*/
struct line {
	time t;
	int amount = 0;
	double price;
};
inline void readline(string& li, line& nextline) {
	string temp;//useless
	int pr, u;
	blank(li);
	istringstream reader(li);
	reader >> temp
		>> nextline.t.h >> nextline.t.m >> u
		>> nextline.price >> nextline.amount;
}
struct AandP {//amount and price
	double amount = 0;
	double price = 0;
};
/*read the file*/
vector<AandP> read(ifstream& in) {
	vector<AandP> proportion(105);//there are 105 intervals
	int sum_amount = 0;
	string temp;
	line current;
	getline(in, temp);
	while (getline(in, temp)) {
		if (temp[0] == '*')
			continue;
		readline(temp, current);
		sum_amount += current.amount;
		proportion[mark(current.t) / 4].amount += current.amount;
		proportion[mark(current.t) / 4].price += current.price;
	}
	for (AandP& p : proportion) {
		//p.price /= p.amount;
		p.amount /= sum_amount;
	}
	return proportion;
}

/*print the distribution*/
void print(vector<AandP>& proportion, ofstream& out, int amount) {
	time time(9);
	out << "distribution:\n" << "---------------------------" << endl;
	double sum = 0;
	for (int i = 0; i < 105; ++i) {
		int min = time.m + xorshf96();
		if (min < 10)
			cout << time.h << ":0" << min
			<< ' ' << int(proportion[i].amount * amount)
			<< " at " << proportion[i].price << endl;
		else 
			cout << time.h << ":" << min
			<< ' ' << int(proportion[i].amount * amount)
			<< " at " << proportion[i].price << endl;
		time = time + 4;
		sum += int(proportion[i].amount * amount)*proportion[i].price;
	}
	cout << "Sum price: " << sum << endl;
	cout << "Average price: " << sum / amount << endl;
}
int main() {
	int amount = 0;
	ifstream in;//to read the file
	ofstream out;//to print the answer
	in.open("600690.iceberg.csv");
	out.open("iceberg.txt");
	vector<AandP> proportion;
	cout << "How large is the order?\n";
	cin >> amount;
	proportion = read(in);
	print(proportion, out, amount);
	in.close();
	out.close();
}