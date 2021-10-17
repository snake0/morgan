#include<iostream>
#include<fstream>
#include<sstream>
#include<vector>
#include<string>
#include<map>

using namespace std;

//--------------------------------------------------------------
//structs

struct quote 
{
	string exchange,//name of exchange
		ssdd,//name of side
		symbol;
	bool side,//0 for Ask, 1 for Bid
		used = 0;//0 for not fill any order;
	unsigned size;
	double price;
	quote(string ex, bool sd, unsigned sz, double pr, string sym)
		:exchange(ex), side(sd), size(sz), price(pr), symbol(sym) {}
	quote() {}
	friend istringstream& operator >> (istringstream&, quote&);
};


struct Exchange 
{
	unsigned quantity = 0;
	double price_sum = 0;
	string Exchange_name;
	Exchange(string name) :Exchange_name(name) {}
	Exchange() {}
	void add_quote(quote&);
};


struct order 
{
	int order_id;
	string symbol;
	bool side,//0 for buy, 1 for sell
		filled = 0;//0 for unfilled
	double price,//the price appeared in "Orders.csv"
		total_price = 0;//the total price of quotes that fill this order
	unsigned quantity,//the quantity appeared in "Orders.csv"
		total_quantity = 0;//the quantity filled by all quotes
	order(int id, bool sd, double pr, unsigned qua, string sym)
		:order_id(id), side(sd), price(pr), quantity(qua), symbol(sym) {}
	order() {}
	friend istringstream& operator >> (istringstream&, order&);
	void add_quote(quote&, ofstream&, int, map<string, Exchange>&);
};



//------------------------------------------------------------------------
//implementation of structs

/*overload of order's operator >> */
istringstream& operator >> (istringstream& iss, order& ord) 
{
	int id;
	bool sd = 0;
	double pr;
	unsigned qua;
	string ssdd, sym;
	iss >> id >> ssdd >> sym >> pr >> qua;
	if (ssdd[0] == 'S')
		sd = 1;
	ord = order(id, sd, pr, qua, sym);
	return iss;
}
/*implementation of Exchange member function add_quote()*/
void Exchange::add_quote(quote& next_quote) 
{
	quantity += next_quote.size;
	price_sum += next_quote.price*next_quote.price;
}
/*overload of quote's operator >> */
istringstream& operator >> (istringstream& iss, quote& qu) 
{
	string ex, symbol;
	bool sd = 0;
	unsigned sz;
	double pr;
	iss >> ex >> qu.ssdd >> symbol >> sz >> pr;
	if (qu.ssdd[0] == 'B')
		sd = 1;
	qu = quote(ex, sd, sz, pr, symbol);
	return iss;
}

/*implementation of order member function add_quote()*/
void order::add_quote(quote& next_quote, ofstream& out,
	int time, map<string, Exchange>& exchanges) 
{
	int sszz;
	double pr;//price to add to total price
	if (side == next_quote.side &&
		(!side && (price > next_quote.price) || side && (price < next_quote.price))
		) {
		next_quote.used = 1;
		pr = !side*price + side*next_quote.price;
		sszz = next_quote.size;
		out << "order_id:" << order_id << "; time:" << time
			<< "; fill quantity:" << sszz << "; "
			<< "fill price:" << next_quote.price << endl;
		total_quantity += sszz;
		total_price += sszz*pr;
		if (total_quantity >= quantity)
			filled = true;
		exchanges[next_quote.exchange].add_quote(next_quote);
	}
}
//--------------------------------------------------------------------------
//functions using the structs

/*make a line readable*/
void add_blank(string& line) {
	for (char& k : line)
		if (k == ',')  k = ' ';
}

/*read a line from quote files or order files*/
template<class T> void read(string& line, istringstream& read_line, T& something) {
	add_blank(line);
	read_line.clear();
	read_line.str(line);
	read_line >> something;
}

/*read all orders from order file*/
void order_processor(vector<order>& orderbook, string& filename) {
	order next_order;
	ifstream in;//to read orders
	istringstream read_line;
	in.open(filename.c_str());
	string line;
	in >> line;
	while (in >> line) {
		read<order>(line, read_line, next_order);
		orderbook.push_back(next_order);
	}
	in.close();
}

/*finish homework 2 & 3*/
void Homework2And3(quote& next_quote,
	ofstream& out, ofstream& passive,
	int& time, int& step,
	vector<order>& orderbook,
	map<string, Exchange>& exchanges,
	vector<int>& level1book)
{
	passive << "\nstep " << time << ':' << endl;
	order order_to_fill;
	for (order& order_to_fill : orderbook) {
		if (order_to_fill.filled)
			continue;
		order_to_fill.add_quote(next_quote, out, time, exchanges);
		if (next_quote.used)
			break;
	}
	int p, sum = 0;
	char check = next_quote.exchange[1],//representing exchange
		company = next_quote.symbol[0];//representing company
	if (check == 'Y')p = 0;
	if (check == 'A')p = 1;
	if (check == 'E')p = 2;
	if (!next_quote.used)
		level1book[p] = next_quote.size;
	else if (order_to_fill.filled)
		level1book[p] = order_to_fill.total_quantity - order_to_fill.quantity;
	for (int k : level1book)
		sum += k;
	for (order k : orderbook) {
		if (!k.filled&&k.symbol[0] == company) {
			passive << "order " << k.order_id << ":\n";
			passive << ((k.quantity - k.total_quantity) / sum)*level1book[0]
				<< '@' << k.price << " on " << "NYSE" << endl;
			passive << ((k.quantity - k.total_quantity) / sum)*level1book[1]
				<< '@' << k.price << " on " << "NASDAQ" << endl;
			passive << ((k.quantity - k.total_quantity) / sum)*level1book[2]
				<< '@' << k.price << " on " << "IEX" << endl;
		}
	}
}

/*read all quotes from quote file*/
void quote_processor(vector<order>& orderbook, string& filename,
	map<string, Exchange>& exchanges) {
	vector<int> level1book(3, 0);//vector第一个元素为NYSE的数量，二为NASDAQ, 三为IEX
	quote next_quote;
	istringstream read_line;
	ifstream in;//to read quotes
	ofstream out,//to store information everytime a quote fills an order
		passive;
	string line;
	in.open(filename.c_str());
	out.open("Order_fill_report.txt");
	passive.open("State of passive placement.txt");
	in >> line;
	int time = 0,//time of the quote
		step = 0;//step of passive model
	while (in >> line) {
		++time;
		read<quote>(line, read_line, next_quote);
		Homework2And3(next_quote, out, passive, time, step, orderbook, exchanges, level1book);
	}
	in.close();
	out.close();
	passive.close();
}

/*read order from console*/
void read_order_from_console(vector<order>& orderbook, istream& in, ostream& out) {
	string si, symbol;
	double pr;
	unsigned qua;
	bool new_side;
	char c1, c2;
	int order_id = orderbook.size() + 1;
	while (1) {
		out << "please enter the order's side, symbol, price,\nquantity seperated by blanks and quit with Enter:\n";
		in.get(c1);
		if (c1 == '\n')
			return;
		in.putback(c1);
		in >> si >> symbol >> pr >> qua;
		if (in.fail()) {
			out << "Invalid order!\n";
			char cleaner;
			while (in.get(cleaner) && cleaner != '\n');
			continue;
		}
		if (si[0] == 'b' || si[0] == 'B')
			new_side = 0;
		else
			new_side = 1;
		orderbook.push_back(order(order_id, new_side, pr, qua, symbol));
		order_id += 1;
		in.get(c2);
	}
}

/*print fill report to console*/
void print_fill_report(vector<order>& orderbook, map<string, Exchange>& exchanges, ostream& out) {
	out << "\n\nOrder fill report:";
	for (map<string, Exchange>::iterator p = exchanges.begin(); p != exchanges.end(); ++p) {
		out << endl << p->first << ":\n";
		out << "quantity: " << p->second.quantity << endl
			<< "average price: " << p->second.price_sum / p->second.quantity << endl;
	}
	cout << endl << "For each order:\n";
	for (order each : orderbook) {
		out << "\nOrder " << each.order_id << ":\n";
		out << "quantity: " << each.total_quantity << endl
			<< "average price: " << each.total_price / each.total_quantity << endl;
	}
}


//--------------------------------------------------------
//main function
int main() {
	vector<order> orderbook;
	string OrderFile = "Orders.csv",
		QuoteFile = "Quotes.csv";
	map<string, Exchange> exchanges;
	exchanges["NYSE"] = Exchange("NYSE");
	exchanges["NASDAQ"] = Exchange("NASDAQ");
	exchanges["IEX"] = Exchange("IEX");
	order_processor(orderbook, OrderFile);
	read_order_from_console(orderbook, cin, cout);
	quote_processor(orderbook, QuoteFile, exchanges);
	print_fill_report(orderbook, exchanges, cout);
}