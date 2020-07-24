#include <iostream>
#include <vector>
int main() {
    std::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    for(auto x : v)
    {
        std::cout<<x<<endl;
    }
    //std::cout << "Hello, World!" << std::endl;
    return 0;
}
