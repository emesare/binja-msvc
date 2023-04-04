// Follow along at: https://www.lukaszlipski.dev/post/rtti-msvc/

struct ParentA
{
	virtual ~ParentA() = default;
	virtual int MyVfuncParentA() { return 4; };
	virtual int MyVfuncParentB() { return 6; };
};

struct ParentB
{
	virtual ~ParentB() = default;
	virtual int MyVfuncParentB() { return 8; };
};

struct SomeClass : ParentA, ParentB
{
	virtual ~SomeClass() = default;
	virtual int getNum() { return 2; }
};

int main()
{
	ParentB* obj = new SomeClass;
	delete obj;
}