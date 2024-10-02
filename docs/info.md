# Info

## Constants

```c
const unsigned long COL_SIG_REV1 = 1;
```

## Types

### RTTICompleteObjectLocator

```c
struct _RTTICompleteObjectLocator {
    unsigned long signature;
    unsigned long offset;
    unsigned long cdOffset;
    // The below are offsets from image base (if signature is set to COL_SIG_REV1).
    int           pTypeDescriptor;
    int           pClassDescriptor;
    // Only present if signature is set to COL_SIG_REV1
    int           pSelf;
}
```

### TypeDescriptor

```c
struct _TypeDescriptor
{
    const void* pVFTable;
    void*       spare;          
    char        name[];
}
```

### RTTIClassHierarchyDescriptor

```c
struct _RTTIClassHierarchyDescriptor {
    unsigned long signature;
    unsigned long attributes;
    unsigned long numBaseClasses;
    int           pBaseClassArray;
}
```

### RTTIBaseClassArray

```c
struct _RTTIBaseClassArray {
    int arrayOfBaseClassDescriptors[];
}
```

### RTTIBaseClassDescriptor

```c
struct _RTTIBaseClassDescriptor {
    int           pTypeDescriptor;
    unsigned long numContainedBases;
    _PMD           where;
    unsigned long attributes;
    int           pClassDescriptor;
}
```

### PMD

```c
struct _PMD
{
    int mdisp;
    int pdisp;
    int vdisp;
}
```
