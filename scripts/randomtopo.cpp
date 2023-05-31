#include <iostream>
#include <iterator>
#include <cmath>
#include <vector>
#include <set>
#include <queue>
#include <algorithm>
using namespace std;
class node{
public:
	double x,y;
	vector<node*> children;
	node* parent;
	int routecount;
	node(double _x, double _y):x(_x),y(_y),parent(NULL),routecount(0){return;}
	double operator-(const node& a){
		return sqrt((x-a.x)*(x-a.x)+(y-a.y)*(y-a.y));
	}
	bool testjoin(node&p){
		node*pp=&p;
		if(p.children.size()+1>15){
			return false;
		}
		while(pp){
			if(
				(pp->parent == NULL && pp->routecount+1>=1000) ||
				(pp->parent != NULL && pp->routecount+1>=10)){
//				cout<<"test connect failed"<<endl;
				return false;
			}
			pp=pp->parent;
		}
		return true;
	}
	void join(node&p){
		parent=&p;
		p.children.push_back(this);
		node*pp=&p;
		while(pp){
			pp->routecount++;
			pp=pp->parent;
		}
	}
};
class topology{
public:
	vector<node> nodes;
	topology():nodes(){}
	vector<node*> testconnect(double d){
		for(auto it=nodes.begin();it!=nodes.end();++it){
			it->routecount=0;
			it->children.clear();
		}
		vector<node*> sequence;
		queue<node*> q;
		sequence.push_back(&*(nodes.begin()));
		q.push(&*(nodes.begin()));
		while(!q.empty()){
			node& c=*(q.front());
			q.pop();
			for(auto it=nodes.begin();it!=nodes.end();++it){
				if(find(sequence.begin(),sequence.end(),&*it)==sequence.end()
					&& c-*it<=d
					&& it->testjoin(c)
					){
					it->join(c);
					sequence.push_back(&*it);
					q.push(&*it);
				}
			}
		}
		return sequence;
	}
};
int idconvert(int x){
	if(x==0)return 1;
	else return x+2;
}
int main(int argc,char**argv){
	if(argc<=1){
		return 1;
	}
	std::uniform_real_distribution<double> unif(-1,1);
	std::default_random_engine re;
	int n = atoi(argv[1]);
	topology t;
	node root(0,0);
	t.nodes.push_back(root);
	for(int i=1;i<n;++i){
		node n(unif(re),unif(re));
		t.nodes.push_back(n);
	}
	double d;
	if(argc>2){
		d = atof(argv[2]);
	}else{
		d = 0.001;
	}
	for(;d<=sqrt(8);d+=0.001){
		auto seq = t.testconnect(d);
		if(seq.size()==t.nodes.size()){
			cout<<d<<endl;
			cout<<"{"<<endl;
			for(auto it=seq.begin()+1;it!=seq.end();++it){
				cout<<"\""<<idconvert(it-seq.begin())<<"\"";
				cout<<":"<<idconvert(find(seq.begin(),seq.end(),(*it)->parent)-seq.begin());
				if(it+1!=seq.end()){
					cout<<",";
				}
				cout<<endl;
			}
			cout<<"}"<<endl;
			cout<<"[";
			for(auto it=seq.begin()+1;it!=seq.end();++it){
				int hop=0;
				for(auto p=(*it)->parent; p; p=p->parent){
					hop++;
				}
				cout<<hop;
				if(it+1!=seq.end()){
					cout<<",";
				}
			}
			cout<<"]"<<endl;
			break;
		}
	}
	
}
