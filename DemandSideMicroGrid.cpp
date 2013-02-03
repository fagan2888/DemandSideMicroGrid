#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <memory.h>
using namespace std;

//Choose Area
#define RESIDENCE 1
//#define INDUSTRY 2
//#define COMMERCIAL 3

#ifdef RESIDENCE
#define NUM_OF_TYPE 14
#define MODE RESIDENCE
#endif

#ifdef INDUSTRY
#define NUM_OF_TYPE 6
#define MODE INDUSTRY
#endif

#ifdef COMMERCIAL
#define NUM_OF_TYPE 8
#define MODE COMMERCIAL
#endif

#ifdef _WIN32
#define ENV 1
#endif

#ifdef __linux
#define ENV 2
#endif

#ifdef MACRO
#define ENV 3
#endif

//Particular Const
#define GENE_PER_POINT NUM_OF_TYPE
#define CHECK_POINT 24

//Basic Const
#define POP_SIZE 50
#define MAX_GENERATION 8000
#define NUM_OF_GENE GENE_PER_POINT * CHECK_POINT
#define MAX_STABLE MAX_GENERATION / 10
#define MODGA_R POP_SIZE / 2
#define P_CROSSOVER 0.7
#define P_MUTATION 0.02
#define P_SUBSTITUTE 0.05
#define ROLL_BOUND 1
#define MAX_LOOP_TIME 10
#define MAX_DELAY 6
#define STAGE 1
#define FACTOR 1000000
#define DATA_HEAD 1
#define DATA_TAIL 10
const string INPUT_ENV_NAME = "inst";
const string INPUT_APP_NAME = "iMnum";
const string OUTPUT_FILE_NAME = "ga_log";

//Particular Variable
class Request
{
public:
	Request()
	{
		amount = 0;
		step = 0;
	}

	Request(int a,int s):amount(a),step(s){}

	int amount;
	int step;
};
Request request[NUM_OF_TYPE][CHECK_POINT];
int device[NUM_OF_TYPE] = {0};
int duration[NUM_OF_TYPE] = {0};
double usage[NUM_OF_TYPE][CHECK_POINT] = {0};
double load[CHECK_POINT] = {0};
double regular[CHECK_POINT] = {0};
double shiftable[CHECK_POINT] = {0};
double price[STAGE + 1][CHECK_POINT] = {0};
double stage[STAGE + 1] = {0};
double total_usage;
int max_duration;

//Basic Variable
int generation;
int best_record;
int stable;
int best_mem;
int worst_mem;

//SA mutation
double adjust[MAX_GENERATION + 1];

class Gene
{
public:
	vector<Request> load;
};

class Genetype
{
public:
	Genetype():fitness(0),rfitness(0),cfitness(0){}
	//Basic Property
	Gene gene[NUM_OF_GENE];
	double fitness;
	double rfitness;
	double cfitness;
	bool chosen;
};
Genetype population[POP_SIZE + 1];
Genetype newpopulation[POP_SIZE + 1];
Genetype history_best;

void initialize(int,int);
void evaluate();
void keep_best();
void elitist();
void select();
void crossover();
void crossover(int,int);
void substitute();
void substitute(int,int);
int compete(int,int);
void mutate();
void rollback();
void swirl(int,Genetype &);
void report(int,string);
void trace(int,string);

void init_file()
{
	//��ȡ��������
	ifstream fin("ga_data0.txt");
	fin.close();
	//�����ض�������ʼ��
	ifstream finf("ga_total.txt");
	finf.close();
}

string convert(int cnt)
{
	if (cnt == 0)
		return "0";
	string str;
	int cpos = 0;
	while (cnt)
	{
		str.insert(str.begin(),1,(char)(cnt % 10 + 48));
		cpos++;
		cnt /= 10;
	}
	return str;
}

void initialize(int cnt)
{
	string dir;
	string dirtyp;
	string str = convert(cnt);
	//ѡ�������ļ�
	switch (MODE)
	{
	case 1:
		if (ENV == 1)
		{
			dir = "residence\\";
			dirtyp = "residence\\resi";
		}
		else
		{
			dir = "residence/";
			dirtyp = "residence/resi";
		}
		break;
	case 2:
		if (ENV == 1)
		{
			dir = "industry\\";
			dirtyp = "industry\\indu";
		}
		else
		{
			dir = "industry/";
			dirtyp = "industry/indu";
		}
		break;
	case 3:
		if (ENV == 1)
		{
			dir = "commercial\\";
			dirtyp = "commercial\\comm";
		}
		else
		{
			dir = "commercial/";
			dirtyp = "commercial/comm";
		}
		break;
	}
	for (int i = 1;i <= STAGE;i++)
		stage[i] = 10000;
	//��ȡ��������
	ifstream fdur(string(dir).append("duration.txt").c_str());
	max_duration = 0;
	for (int i = 0;i < NUM_OF_TYPE;i++)
	{
		fdur >> duration[i];
		if (duration[i] > max_duration)
			max_duration = duration[i];
	}
	fdur.close();
	ifstream ftyp(dirtyp.append("type.txt").c_str());
	for (int i = 0;i < NUM_OF_TYPE;i++)
	{
		for (int j = 0;j < max_duration;j++)
			ftyp >> usage[i][j];
		ftyp >> device[i];
	}
	ftyp.close();
	ifstream fenv(string(dir).append(str).append(INPUT_ENV_NAME).append(".txt").c_str());
	int point;
	total_usage = 0;
	for (int i = 0;i < CHECK_POINT;i++)
	{
		fenv >> point;
		for (int j = 1;j <= STAGE;j++)
			fenv >> price[j][point];
		fenv >> shiftable[point] >> regular[point];
		total_usage += shiftable[point] + regular[point];
	}
	fenv.close();
	ifstream fapp(string(dir).append(str).append(INPUT_APP_NAME).append(".txt").c_str());
	for (int i = 0;i < CHECK_POINT;i++)
		for (int j = 0;j < NUM_OF_TYPE;j++)
		{
			Request temp;
			fapp >> temp.amount;
			request[j][i] = temp;
		}
	fapp.close();
	//��ʼ��Ⱦɫ��
	for (int i = 0;i < POP_SIZE;i++)
	{
		double loaded[CHECK_POINT] = {0};
		for (int j = 0;j < NUM_OF_GENE;j++)
			population[i].gene[j].load.clear();
		for (int j = 0;j < CHECK_POINT;j++)
			loaded[j] = regular[j];
		for (int j = 0;j < CHECK_POINT;j++)
		{
			for (int k = 0;k < NUM_OF_TYPE;k++)
			{
				int pos = j * GENE_PER_POINT + k;
				//�����������Ϊ������Ҫ������д���
				if (request[k][j].amount > 0)
					population[i].gene[pos].load.push_back(request[k][j]);
				else continue;
				int type = k,amount,step,remain;
				Request temp = population[i].gene[pos].load.back();
				while (true)
				{
					double current[CHECK_POINT] = {0};
					//ʣ����ӳ�ʱ��
					remain = MAX_DELAY - temp.step;
					//���㵱ǰ����Ӧ�õ�ĳ��ʱ�̺���õ����ĸı�̶�
					for (int p = 0;p < duration[type];p++)
						current[p] = temp.amount * usage[type][p];
					if (temp.step == MAX_DELAY)
					{
						amount = temp.amount;
						step = 0;
					} 
					else if (temp.amount > 0)
					{
						amount = rand() % (temp.amount + 1);
						double possible[CHECK_POINT + 1] = {0};
						int t;
						//���㵱ǰ����Ӧ�õ�ĳ��ʱ�̺󣬸�ʱ���õ���ռ���õ����ı���
						for (t = 0;t <= remain;t++)
							for (int p = 0;p < duration[type];p++)
							{
								int s;
								double use = loaded[(j + temp.step + t + p) % CHECK_POINT] + current[p];
								for (s = 1;s <= STAGE;s++)
									if (use >= stage[s - 1] && use < stage[s])
										break;
								possible[t + 1] += use * price[s][(j + temp.step + t + p) % CHECK_POINT];
							}
						double sum = 0;
						for (int p = 1;p <= t;p++)
						{
							for (int s = 1;s <= STAGE;s++)
								if (possible[p] >= stage[s - 1] && possible[p] < stage[s])
								{
									possible[p] = total_usage / possible[p];
									sum += possible[p];
									break;
								}
						}
						//����ÿ��ʱ���õ���ռ�ı�����������ȷ�������Ӻ��ʱ��
						for (int p = 1;p <= t;p++)
							possible[p] = possible[p] / sum + possible[p - 1];
						double roll = rand() % 10000 / 10000.0;
						for (int p = 1;p <= t;p++)
							if (roll >= possible[p - 1] && roll < possible[p])
								step = p - 1;
					}
					//���Ӻ��豸���������Ӻ��ʱ�䲻Ϊ0���򽫵�ǰ������Ϊ������һ���ڵ�ǰʱ��ִ�У�һ���Ӻ�stepСʱִ��
					if (amount > 0 && step > 0 && temp.step < MAX_DELAY)
					{
						population[i].gene[pos].load.back().amount -= amount;
						population[i].gene[pos].load.push_back(Request(amount,temp.step + step));
					}
					else break;
					temp = population[i].gene[pos].load.back();
				}
			}
		}
	}
	population[POP_SIZE] = population[0];
}

void evaluate()
{
	double sum;
	int i,j,k,p,m;
	for (i = 0;i <= POP_SIZE;i++)
	{
		sum = 0;
		memset(load,0,sizeof(load));
		for (j = 0;j < CHECK_POINT;j++)
		{
			for (k = 0;k < NUM_OF_TYPE;k++)
			{
				int pos = j * GENE_PER_POINT + k;
				for (p = 0;p < population[i].gene[pos].load.size();p++)
				{
					Request temp = population[i].gene[pos].load[p];
					for (m = 0;m < duration[k];m++)
						load[(j + m + temp.step) % CHECK_POINT] += temp.amount * usage[k][m];
				}
			}
			double total = load[j] + regular[j];
			for (k = 1;k < STAGE;k++)
				if (total >= stage[k - 1] && total < stage[k])
					break;
			sum += total * price[k][j];
		}
		//����Ҫ������С��ѣ�����õ�ѵĵ�����Ϊ��Ӧֵ��FACTOR��������֤���ȵĳ���������ļ��м�¼�Ĳ�����Ӧֵ�����ܵ��
		population[i].fitness = FACTOR / sum;
	}
}

void keep_best()
{
	int mem,i;
	best_record = 0;
	for (mem = 0;mem < POP_SIZE;mem++)
	{
		if (population[mem].fitness > population[POP_SIZE].fitness)
		{
			best_record = mem;
			population[POP_SIZE].fitness = population[mem].fitness;
		}
	}
	for (i = 0;i < NUM_OF_GENE;i++)
		population[POP_SIZE].gene[i] = population[best_record].gene[i];
}

void elitist()
{
	int i;
	double best,worst;
	best = population[0].fitness;
	worst = population[0].fitness;
	for (i = 0;i < POP_SIZE - 1;i++)
	{
		if (population[i].fitness > population[i + 1].fitness)
		{
			if (population[i].fitness >= best)
			{
				best = population[i].fitness;
				best_mem = i;
			}
			if (population[i + 1].fitness <= worst)
			{
				worst = population[i + 1].fitness;
				worst_mem = i + 1;
			}
		}
		else
		{
			if (population[i].fitness <= worst)
			{
				worst = population[i].fitness;
				worst_mem = i;
			}
			if (population[i + 1].fitness >= best)
			{
				best = population[i + 1].fitness;
				best_mem = i + 1;
			}
		}
	}
	if (best > population[POP_SIZE].fitness)
	{
		for (i = 0;i < NUM_OF_GENE;i++)
			population[POP_SIZE].gene[i] = population[best_mem].gene[i];
		population[POP_SIZE].fitness = population[best_mem].fitness;
		stable = 0;
	}
	else
	{
		for (i = 0;i < NUM_OF_GENE;i++)
			population[worst_mem].gene[i] = population[POP_SIZE].gene[i];
		population[worst_mem].fitness = population[POP_SIZE].fitness;
		stable++;
	}
}

void select()
{
	int first = 0,mem,i,j,one;
	double sum = 0,p,x;
	for (mem = 0;mem < POP_SIZE;mem++)
		sum += population[mem].fitness;
	for (mem = 0;mem < POP_SIZE;mem++)
	{
		population[mem].rfitness = population[mem].fitness / sum;
		population[mem].chosen = false;
	}
	population[0].cfitness = population[0].rfitness;
	for (mem = 1;mem < POP_SIZE;mem++)
		population[mem].cfitness = population[mem - 1].cfitness + population[mem].rfitness;
	/*for (i = 0;i < POP_SIZE;i++)
	{
		p = rand() % 1000 / 1000.0;
		if (p < population[0].cfitness)
			newpopulation[i] = population[0];
		else
			for (j = 0;j < POP_SIZE;j++)
				if (p >= population[j].cfitness && p < population[j + 1].cfitness)
					newpopulation[i] = population[j + 1];
	}*/
	for (i = 0;i < MODGA_R;i++)
	{
		p = rand() % 1000 / 1000.0;
		if (p < population[0].cfitness)
			newpopulation[i] = population[0];
		else
			for (j = 0;j < POP_SIZE;j++)
				if (p >= population[j].cfitness && p < population[j + 1].cfitness)
					newpopulation[i] = population[j + 1];
	}
	for (mem = 0;mem < MODGA_R;mem++)
	{
		x = rand() % 1000 / 1000.0;
		if (x < P_CROSSOVER)
		{
			first++;
			if (first % 2 == 0)
				newpopulation[mem] = newpopulation[compete(one,mem)];
			else one = mem;
		}
	}
	for (i = MODGA_R;i < POP_SIZE;i++)
	{
		p = rand() % 1000 / 1000.0;
		if (p < population[0].cfitness)
		{
			if (!population[0].chosen)
			{
				newpopulation[i] = population[0];
				population[0].chosen = true;
			}
			else i--;
		}
		else
			for (j = 0;j < POP_SIZE;j++)
				if (p >= population[j].cfitness && p < population[j + 1].cfitness)
				{
					if (!population[j + 1].chosen)
					{
						newpopulation[i] = population[j + 1];
						population[j + 1].chosen = true;
					}
					else i--;
				}
	}
	/*
	for (i = 0;i < POP_SIZE;i++)
	{
		population[i] = newpopulation[i];
		swirl(i,newpopulation[i]);
	}
	*/
}

void crossover()
{
	int first = 0,mem,one;
	double x;
	for (mem = 0;mem < POP_SIZE;mem++)
	{
		x = rand() % 1000 / 1000.0;
		if (x < P_CROSSOVER)
		{
			first++;
			if (first % 2 == 0)
				crossover(one,mem);
			else one = mem;
		}
	}
}

void crossover(int one,int two)
{
	int i,a,b;
	a = rand() % (NUM_OF_GENE - 1) + 1;
	b = rand() % (NUM_OF_GENE - a) + a;
	for (i = a;i < b;i++)
		swap(population[one].gene[i],population[two].gene[i]);
}

void substitute()
{
	int first = 0,mem,one;
	double x;
	for (mem = 0;mem < POP_SIZE;mem++)
	{
		x = rand() % 1000 / 1000.0;
		if (x < P_CROSSOVER)
		{
			first++;
			if (first % 2 == 0)
				substitute(one,mem);
			else one = mem;
		}
	}
}

void substitute(int one,int two)
{
	int i;
    double x;
	for (i = 0;i < NUM_OF_GENE;i++)
	{
        x = rand() % 1000 / 1000.0;
        if (x < P_SUBSTITUTE)
            population[one].gene[i] = population[two].gene[i];
	}
}

int compete(int one,int two)
{
	double p;
	if (population[one].fitness < population[two].fitness)
		return one;
	else
	{
		p = rand() % 1000 / 1000.0;
		if (p > 1 / (1 + exp((population[one].fitness + population[two].fitness) / generation)))
			return two;
		else return one;
	}
}

void mutate()
{
	int i,j;
	double x;
	for (i = 0;i < POP_SIZE;i++)
		for (j = 0;j < NUM_OF_GENE;j++)
		{
			x = (rand() % 1000) / 1000.0;
			if (x < P_MUTATION * adjust[generation])
			{
				//�����ʵ��˼�������ѡ��һ�����򣬼����ѡ��ĳ���豸��ĳ��ʱ�̵�������Ӧ���������÷������½��м���
				double loaded[CHECK_POINT] = {0};
				int type = j % GENE_PER_POINT,point = j / GENE_PER_POINT,amount,step,remain;
				population[i].gene[j].load.clear();
				population[i].gene[j].load.push_back(request[type][point]);
				Request temp = population[i].gene[j].load.back();
				while (true)
				{
					double current[CHECK_POINT] = {0};
					remain = MAX_DELAY - temp.step;
					for (int p = 0;p < duration[type];p++)
						current[p] = temp.amount * usage[type][p];
					if (temp.step == MAX_DELAY)
					{
						amount = temp.amount;
						step = 0;
					} 
					else if (temp.amount > 0)
					{
						amount = rand() % (temp.amount + 1);
						double possible[CHECK_POINT + 1] = {0};
						int t;
						for (t = 0;t <= remain;t++)
							for (int p = 0;p < duration[type];p++)
							{
								int s;
								double use = loaded[(j + temp.step + t + p) % CHECK_POINT] + current[p];
								for (s = 1;s <= STAGE;s++)
									if (use >= stage[s - 1] && use < stage[s])
										break;
								possible[t + 1] += use * price[s][(j + temp.step + t + p) % CHECK_POINT];
							}
						double sum = 0;
						for (int p = 1;p <= t;p++)
						{
							for (int s = 1;s <= STAGE;s++)
								if (possible[p] >= stage[s - 1] && possible[p] < stage[s])
								{
									possible[p] = total_usage / possible[p];
									sum += possible[p];
									break;
								}
						}
						for (int p = 1;p <= t;p++)
							possible[p] = possible[p] / sum + possible[p - 1];
						double roll = rand() % 10000 / 10000.0;
						for (int p = 1;p <= t;p++)
							if (roll >= possible[p - 1] && roll < possible[p])
								step = p - 1;
					}
					if (amount > 0 && step > 0 && temp.step < MAX_DELAY)
					{
						population[i].gene[j].load.back().amount -= amount;
						population[i].gene[j].load.push_back(Request(amount,temp.step + step));
					}
					else break;
					temp = population[i].gene[j].load.back();
				}
			}
		}
}

void rollback()
{
	int i,j;
	for (i = 0;i < MODGA_R / 2;i++)
	{
		j = rand() % POP_SIZE;
		swirl(j,population[POP_SIZE]);
	}
	for (i = POP_SIZE - MODGA_R / 2;i < POP_SIZE;i++)
	{
		j = rand() % POP_SIZE;
		swirl(j,history_best);
	}
	for (i = 0;i < POP_SIZE / 2;i++)
	{
		j = rand() % POP_SIZE;
		substitute(j,POP_SIZE);
		crossover(j,POP_SIZE);
	}
	population[POP_SIZE] = population[0];
}

//������������δʹ��
void swirl(int index,Genetype & base)
{
	int i,p;
	for (i = 0;i < NUM_OF_GENE;i++)
	{
		p = rand() % (ROLL_BOUND * 2 + 1) - ROLL_BOUND;
		for (int j = 0;j < population[index].gene[i].load.size() && j < base.gene[i].load.size();j++)
			population[index].gene[i].load[j].step = (base.gene[i].load[j].step + p + MAX_DELAY + 1) % (MAX_DELAY + 1);
	}
}

void report(int cnt,string prefix)
{
	ofstream fout(prefix.append(OUTPUT_FILE_NAME).append(convert(cnt)).append(".txt").c_str(),ios::app);
	/*fout << generation << "\n";
	for (int i = 0;i < NUM_OF_CITY;i++)
	{
		fout << i + 1 << ": \n";
		for (int j = i;j < NUM_OF_GENE;j += NUM_OF_CITY)
			fout << population[POP_SIZE].gene[j].borrow_city + 1 << " " << population[POP_SIZE].gene[j].borrow_amount << "\n";
	}*/
	fout << fixed << setprecision(4) << FACTOR / population[POP_SIZE].fitness << "\n";
	fout.close();
}

void trace(int cnt,string prefix)
{
	ofstream fout(prefix.append(OUTPUT_FILE_NAME).append(convert(cnt)).append(".txt").c_str(),ios::app);
	/*fout << generation << "\n";
	for (int i = 0;i < NUM_OF_CITY;i++)
	{
		fout << i + 1 << ": \n";
		for (int j = i;j < NUM_OF_GENE;j += NUM_OF_CITY)
			fout << population[POP_SIZE].gene[j].borrow_city + 1 << " " << population[POP_SIZE].gene[j].borrow_amount << "\n";
	}
	for (int i = 0;i < NUM_OF_GENE;i++)
		fout << population[POP_SIZE].gene[i].borrow_amount << "\n";*/
	fout << fixed << setprecision(4) << FACTOR / history_best.fitness << "\n";
	fout.close();
}

int main()
{
	int count;
	clock_t start,end;
	string progress_bar,prefix;
	double complete_percent;
	for (int i = 1;i <= MAX_GENERATION;i++)
		adjust[i] = 1 + exp(float(-i));
	srand((unsigned int)time(0));
	start = clock();
	for (int i = DATA_HEAD;i <= DATA_TAIL;i++)
	{
		history_best = Genetype();
		count = 0;
		complete_percent = 0;
		progress_bar.clear();
		switch (MODE)
		{
		case 1:
			prefix = string("resi").append(convert(i)).append("_step").append(convert(MAX_DELAY));
			if (ENV == 1)
				prefix.append("\\");
			else prefix.append("/");
			break;
		case 2:
			prefix = string("indu").append(convert(i)).append("_step").append(convert(MAX_DELAY));
			if (ENV == 1)
				prefix.append("\\");
			else prefix.append("/");
			break;
		case 3:
			prefix = string("comm").append(convert(i)).append("_step").append(convert(MAX_DELAY));
			if (ENV == 1)
				prefix.append("\\");
			else prefix.append("/");
			break;
		}
		printf("0%%\n");
		//init_file();
		while (count < MAX_LOOP_TIME)
		{
			progress_bar.append(">");
			complete_percent = (double)(count + 1) / (double)MAX_LOOP_TIME * 100;
			generation = 0;
			initialize(i);
			evaluate();
			keep_best();
			while (generation++ < MAX_GENERATION)
			{
				elitist();
				select();
				report(count,prefix);
				crossover();
				mutate();
				if (stable >= MAX_STABLE)
					rollback();
				if (population[POP_SIZE].fitness > history_best.fitness)
					history_best = population[POP_SIZE];
				evaluate();
			}
			trace(MAX_LOOP_TIME,prefix);
			//cout << progress_bar << fixed << setprecision(2) << complete_percent << "%\n";
            printf("%s%.2f%%\n",progress_bar.c_str(),complete_percent);
			count++;
		}
		
		ofstream ftotal(string(prefix).append("ga_bestgene.txt").c_str());
		int j;
		for (j = 0;j < NUM_OF_GENE;j++)
		{
			for (int k = 0;k < history_best.gene[j].load.size();k++)
				if (history_best.gene[j].load[k].amount > 0)
					ftotal << history_best.gene[j].load[k].amount << " " << history_best.gene[j].load[k].step << endl;
			if (history_best.gene[j].load.size() > 0)
				ftotal << endl;
			else ftotal << "0\n";
		}
		ftotal.close();

		ofstream fuse(string(prefix).append("ga_use.txt").c_str());
		double sum;
		int i,k,p;
		memset(load,0,sizeof(load));
		for (j = 0;j < CHECK_POINT;j++)
			for (k = 0;k < NUM_OF_TYPE;k++)
			{
				int pos = j * GENE_PER_POINT + k;
				for (p = 0;p < history_best.gene[pos].load.size();p++)
				{
					Request temp = history_best.gene[pos].load[p];
					for (int m = 0;m < duration[k];m++)
						load[(j + m + temp.step) % CHECK_POINT] += temp.amount * usage[k][m];
				}
			}
		for (i = 0;i < CHECK_POINT;i++)
			fuse << load[i] << endl;
		fuse.close();
	}
	
	end = clock();
	//cout << "\nCompleted.\nTime used: " << (double)(end - start) / (CLOCKS_PER_SEC * 60) << " min\n";
    printf("\nCompleted.\nTime used: %.2f min",(double)(end - start) / (CLOCKS_PER_SEC * 60));
	return 0;
}