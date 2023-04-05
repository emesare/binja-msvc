# MSVC RTTI

Parses and symbolizes MSVC RTTI information in [Binary Ninja].

## Example Listing

Arguably the most import function of symbolizing RTTI information is the virtual function tables. The listing below is the symbolized view of `simple.cpp` (found in test\bins).

```c
void* data_140010320 = ParentA_objLocator
struct ParentA_vfTable = 
{
    void* (* const vFunc_0)(void* arg1, int32_t arg2) = ParentB::vFunc_0
    int64_t (* const vFunc_1)() = ParentA::vFunc_1
    int64_t (* const vFunc_2)() = ParentA::vFunc_2
}
void* data_140010340 = ParentB_objLocator
struct ParentB_vfTable = 
{
    void* (* const vFunc_0)(void* arg1, int32_t arg2) = ParentB::vFunc_0
    int64_t (* const vFunc_1)() = ParentB::vFunc_1
}
void* data_140010358 = SomeClass_objLocator
struct SomeClass_vfTable = 
{
    void* (* const vFunc_0)(void* arg1, int32_t arg2) = SomeClass::vFunc_0
    int64_t (* const vFunc_1)() = ParentA::vFunc_1
    int64_t (* const vFunc_2)() = ParentA::vFunc_2
    int64_t (* const vFunc_3)() = SomeClass::vFunc_3
}
void* data_140010380 = SomeClass_objLocator
struct SomeClass_vfTable = 
{
    int64_t (* const vFunc_0)(int64_t arg1, int32_t arg2) = SomeClass::vFunc_0
    int64_t (* const vFunc_1)() = ParentB::vFunc_1
}
void* data_140010398 = type_info_objLocator
struct type_info_vfTable = 
{
    void*** (* const vFunc_0)(void*** arg1, char arg2) = type_info::vFunc_0
}
void* data_1400103a8 = std::exception_objLocator
struct std::exception_vfTable = 
{
    void*** (* const vFunc_0)(void*** arg1, char arg2) = std::exception::vFunc_0
    int64_t (* const vFunc_1)() = std::exception::vFunc_1
}
```

## TODO

- ~~Identify virtual functions~~ and integrate with component view.
- Provide a UI to view associated classes.
- Graphviz support.
- Automatic scan on binary open.
- Provide CI for releasing new versions automatically and on all platforms.
- Provide better logging.
- Fixup cross references between defined symbols and their RVA pointers.
- Provide statistics on discovered functions and other useful information after completion.

[Binary Ninja]: https://binary.ninja
