#include <stdint.h>

struct ParentA
{
	virtual ~ParentA() = default;
	uint64_t ParentAValue;
};

struct ParentB
{
	virtual ~ParentB() = default;
	uint32_t ParentBValue;
};

struct MultiSomeClass : ParentA, ParentB
{
	virtual ~MultiSomeClass() = default;
	uint16_t MultiSomeClassValue;
};

struct SomeClass : ParentA
{
	virtual ~SomeClass() = default;
	uint16_t SomeClassValue;
};

void someClass()
{
	SomeClass* obj = new SomeClass();
	obj->SomeClassValue = 33;
	obj->ParentAValue = 55;
}

void multiSomeClass()
{
	MultiSomeClass* obj = new MultiSomeClass();
	obj->MultiSomeClassValue = 22;
	obj->ParentAValue = 44;
	obj->ParentBValue = 77;
}

int main()
{
	someClass();
	multiSomeClass();
}