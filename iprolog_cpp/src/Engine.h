#ifndef __ENGINE_H__
#define __ENGINE_H__
//including the libraries
#include <string>
#include <vector>
#include <map>
//including the dependencies
#include "Clause.h"
#include "IntStack.h"
#include "ObStack.h"
#include "Spine.h"
#include "IMap.h"
#include "Object.h"


class Engine
{
public:
	static const int MAXIND = 3;
	static const int START_INDEX = 20;
	// switches off indexing for less then START_INDEX clauses e.g. <20

	Engine(std::string fname);
	~Engine();

	/**
	* initiator and consumer of the stream of answers
	* generated by this engine
	*/
	void run();


private:
   // flag for export term so it doesn't waste time if we are not printing out answers.
   bool exportTermFlag_ = true;

   /**
	* trimmed down clauses ready to be quickly relocated to the heap
	*/
	std::vector<Clause>* clauses;
	std::vector<int>* cls;

	/** symbol table made of map + reverse map from ints to syms */
	std::map<std::string, int> syms;
	std::vector<std::string> slist;

	/** runtime areas:
	*
	* the heap contains code for and clauses their their copies
	* created during execution
	*
	* the trail is an undo list for variable bindings
	* that facilitates retrying failed goals with alternative
	* matching clauses
	*
	* the unification stack ustack helps handling term unification non-recursively
	*
	* the spines stack contains abstractions of clauses and goals and performs the
	* functions of  both a choice-point stack and goal stack
	*
	* imaps: contains indexes for up toMAXIND>0 arg positions (0 for pred symbol itself)
	*
	* vmaps: contains clause numbers for which vars occur in indexed arg positions
	*/

	std::vector<int> heap;
	int top;
	static const int MINSIZE = 1 << 15;
	IntStack trail;
	IntStack ustack;
	ObStack<Spine*> spines;
	Spine* query;
	std::vector<IMap<int>*>* imaps;
	std::vector<IntMap*>* vmaps;

	static const int V = 0;
	static const int U = 1;
	static const int R = 2;

	static const int C = 3;
	static const int N = 4;

	static const int A = 5;

	// G - ground?

	static const int BAD = 7;


	/**
	* tags an integer value while fliping it into a negative
	* number to ensure that untagged cells are always negative and the tagged
	* ones are always positive - a simple way to ensure we do not mix them up
	* at runtime
	*/
	static int tag(int t, int w)
	{
		return -((w << 3) + t);
	}

	/**
	* removes tag after flipping sign
	*/
	static int detag(int w)
	{
		return -w >> 3;
	}

	/**
	* extracts the tag of a cell
	*/
	static int tagOf(int w)
	{
		return -w & 7;
	}

	/**
	* places an identifier in the symbol table
	*/
	int addSym(std::string sym);

	/**
	* returns the symbol associated to an integer index
	* in the symbol table
	*/
	std::string getSym(int w);

	void makeHeap()
	{
		makeHeap(MINSIZE);
	}

	void makeHeap(int size)
	{
		heap = std::vector<int>(size, 0);
		clear();
	}

	int getTop()
	{
		return top;
	}

	int setTop(int top)
	{
		return this->top = top;
	}

	void clear()
	{
		top = -1;
	}

	/**
	* Pushes an element - top is incremented frirst than the
	* element is assigned. This means top point to the last assigned
	* element - which can be returned with peek().
	*/
	void push(int i)
	{
		heap[++top] = i;
	}

	int size()
	{
		return top + 1;
	}

	/**
	* dynamic array operation: doubles when full
	*/
	void expand();

	void ensureSize(int more)
	{
		if (1U + top + more >= heap.size())
			expand();
	}

	/**
	* expands a "Xs lists .." statements to "Xs holds" statements
	*/
	static std::vector<std::vector<std::string>*>* maybeExpand(std::vector<std::string>& Ws);


	/**
	* expands, if needed, "lists" statements in sequence of statements
	*/
	static std::vector<std::vector<std::string>*>* mapExpand(std::vector<std::vector<std::string>*>& Wss);

	/**
	* loads a program from a .nl file of
	* "natural language" equivalents of Prolog/HiLog statements
	*/
	std::vector<Clause>* dload(std::string s);

	static std::vector<int>* toNums(std::vector<Clause>* clauses);


	/**
	* encodes string constants into symbols while leaving
	* other data types untouched
	*/
	int encode(int t, std::string s);

	/**
	* true if cell x is a variable
	* assumes that variables are tagged with 0 or 1
	*/
	static bool isVAR(int x)
	{
		return tagOf(x) < 2;
	}

	/**
	* returns the heap cell another cell points to
	*/
	int getRef(int x)
	{
		return heap[detag(x)];
	}

	/*
	* sets a heap cell to point to another one
	*/
	void setRef(int w, int r)
	{
		heap[detag(w)] = r;
	}


	/**
	* removes binding for variable cells
	* above savedTop
	*/
	void unwindTrail(int savedTop)
	{
		while (savedTop < trail.getTop()) {
			int href = trail.pop();
			// assert href is var

			setRef(href, href);
		}
	}

	/**
	* scans reference chains starting from a variable
	* until it points to an unbound root variable or some
	* non-variable cell
	*/
	int deref(int x);


	/**
	* raw display of a term - to be overridden
	*/
	std::string showTerm(int x) {
		return showTerm(exportTerm(x));
	}

	/**
	* raw display of a externalized term
	*/
	std::string showTerm(Object O);


	/**
	* builds an array of embedded arrays from a heap cell
	* representing a term for interaction with an external function
	* including a displayer
	*/
	Object exportTerm(int x);



	/**
	* extracts an integer array pointing to
	* the skeleton of a clause: a cell
	* pointing to its head followed by cells pointing to its body's
	* goals
	*/
	static std::vector<int>* getSpine(std::vector<int>& cs);

	/**
	* raw display of a cell as tag : value
	*/
	std::string showCell(int w);

	/**
	* a displayer for cells
	*/
	std::string showCells(int base, int len);


	

	/**
	* unification algorithm for cells X1 and X2 on ustack that also takes care
	* to trail bindigs below a given heap address "base"
	*/
	bool unify(int base);
	bool unify_args(int w1, int w2);

	/**
	* places a clause built by the Toks reader on the heap
	*/
	Clause putClause(std::vector<int> cs, std::vector<int>& gs, int neck);


	/**
	* relocates a variable or array reference cell by b
	* assumes var/ref codes V,U,R are 0,1,2
	*/
	static int relocate(int b, int cell)
	{
		return tagOf(cell) < 3 ? cell + b : cell;
	}


	/**
	* pushes slice[from,to] of array cs of cells to heap
	*/
	void pushCells(int b, int from, int to, int base);

	/**
	* pushes slice[from,to] of array cs of cells to heap
	*/
	void pushCells(int b, int from, int to, std::vector<int>& cs);


	/**
	* copies and relocates head of clause at offset from heap to heap
	*/
	int pushHead(int b, Clause& C);

	/**
	* copies and relocates body of clause at offset from heap to heap
	* while also placing head as the first element of array gs that
	* when returned contains references to the toplevel spine of the clause
	*/
	std::vector<int>* pushBody(int b, int head, Clause& C);

	/**
	* makes, if needed, registers associated to top goal of a Spine
	* these registers will be reused when matching with candidate clauses
	* note that xs contains dereferenced cells - this is done once for
	* each goal's toplevel subterms
	*/
	void makeIndexArgs(Spine& G, int goal);


	std::vector<int>* getIndexables(int ref);

	int cell2index(int cell);

	/**
	* tests if the head of a clause, not yet copied to the heap
	* for execution could possibly match the current goal, an
	* abstraction of which has been place in xs
	*/
	bool match(std::vector<int>& xs, Clause& C0);


	/**
	* transforms a spine containing references to choice point and
	* immutable list of goals into a new spine, by reducing the
	* first goal in the list with a clause that successfully
	* unifies with it - in which case places the goals of the
	* clause at the top of the new list of goals, in reverse order
	*/
	Spine* unfold(Spine& G);

	/**
	* extracts a query - by convention of the form
	* goal(Vars):-body to be executed by the engine
	*/
	Clause* getQuery()
	{
		return &(*clauses)[clauses->size() - 1];
	}

	/**
	* returns the initial spine built from the
	* query from which execution starts
	*/
	Spine* init();

	/**
	* returns an answer as a Spine while recording in it
	* the top of the trail to allow the caller to retrieve
	* more answers by forcing backtracking
	*/
	Spine* answer(int ttop)
	{
		return new Spine(spines[0]->hd, ttop);
	}

	/**
	* detects availability of alternative clauses for the
	* top goal of this spine
	*/
	bool hasClauses(Spine& S)
	{
		return S.k < (int)S.cs->size();
	}

	/**
	* true when there are no more goals left to solve
	*/
	bool hasGoals(Spine& S)
	{
		return !IntList::isEmpty(S.gs);
	}

	/**
	* removes this spines for the spine stack and
	* resets trail and heap to where they where at its
	* creating time - while undoing variable binding
	* up to that point
	*/
	void popSpine();

	/**
	* main interpreter loop: starts from a spine and works
	* though a stream of answers, returned to the caller one
	* at a time, until the spines stack is empty - when it
	* returns null
	*/
	Spine* yield();

	/**
	* retrieves an answers and ensure the engine can be resumed
	* by unwinding the trail of the query Spine
	* returns an external "human readable" representation of the answer
	*/
	Object ask();



	// indexing extensions - ony active if START_INDEX clauses or more
	static std::vector<IntMap*>* vcreate(int l);

	void put(std::vector<IMap<int>*>* imaps, std::vector<IntMap*>* vss, std::vector<int>* keys, int val);

	std::vector<IMap<int>*>* index(std::vector<Clause>* clauses, std::vector<IntMap*>* vmaps);



};






#endif
