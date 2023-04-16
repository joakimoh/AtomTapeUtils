#include "../shared/utility.h"
#include <iostream>
#include <string>

using namespace std;

int main(int argc, const char* argv[])
{
	vector<int> v1 = { 1, 2, 3, 4, 5 };
	vector<int> v2(5);

	vector<int>::iterator v1_iter = v1.begin();
	vector<int>::iterator v2_iter = v2.begin();

	for (int i = 0; i < v1.size();i++)
		v2[i] = v1_iter[i];

	for (int i = 0; i < v2.size(); i++)
		cout << v2[i] << "\n";
	
}
