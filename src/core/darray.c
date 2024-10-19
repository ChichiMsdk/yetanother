#include "darray.h"
#include "ymemory.h"
#include "logger.h"

#include <string.h>
#include <stdio.h>

void*
_DarrayCreate(uint64_t length, uint64_t stride)
{
    uint64_t headerSize = DARRAY_FIELD_LENGTH * sizeof(uint64_t);
    uint64_t arraySize = length * stride;
    uint64_t* pNewArray = yAlloc(headerSize + arraySize, MEMORY_TAG_DARRAY);
    memset(pNewArray, 0, headerSize + arraySize);
    pNewArray[DARRAY_CAPACITY] = length;
    pNewArray[DARRAY_LENGTH] = 0;
    pNewArray[DARRAY_STRIDE] = stride;
    return pNewArray + DARRAY_FIELD_LENGTH;
}

void
_DarrayDestroy(void* pArray)
{
    uint64_t* header = (uint64_t*)pArray - DARRAY_FIELD_LENGTH;
    uint64_t header_size = DARRAY_FIELD_LENGTH * sizeof(uint64_t);
    uint64_t total_size = header_size + header[DARRAY_CAPACITY] * header[DARRAY_STRIDE];
    yFree(header, total_size, MEMORY_TAG_DARRAY);
}

uint64_t
_DarrayFieldGet(void* pArray, uint64_t field)
{
    uint64_t* header = (uint64_t*)pArray - DARRAY_FIELD_LENGTH;
    return header[field];
}

void
_DarrayFieldSet(void* pArray, uint64_t field, uint64_t value)
{
    uint64_t* pHeader = (uint64_t*)pArray - DARRAY_FIELD_LENGTH;
    pHeader[field] = value;
}

void*
_DarrayResize(void* pArray)
{
    uint64_t length = darray_length(pArray);
    uint64_t stride = darray_stride(pArray);
    void* temp = _DarrayCreate(
        (DARRAY_RESIZE_FACTOR * darray_capacity(pArray)),
        stride);
    memcpy(temp, pArray, length * stride);

    _DarrayFieldSet(temp, DARRAY_LENGTH, length);
    _DarrayDestroy(pArray);
    return temp;
}

void*
_DarrayPush(void* pArray, const void* pValue)
{
    uint64_t length = darray_length(pArray);
    uint64_t stride = darray_stride(pArray);
    if (length >= darray_capacity(pArray))
	{
        pArray = _DarrayResize(pArray);
    }

    uint64_t addr = (uint64_t)pArray;
    addr += (length * stride);
    memcpy((void*)addr, pValue, stride);
    _DarrayFieldSet(pArray, DARRAY_LENGTH, length + 1);
    return pArray;
}

void
_DarrayPop(void* pArray, void* pDest)
{
    uint64_t length = darray_length(pArray);
    uint64_t stride = darray_stride(pArray);
    uint64_t addr = (uint64_t)pArray;
    addr += ((length - 1) * stride);
    memcpy(pDest, (void*)addr, stride);
    _DarrayFieldSet(pArray, DARRAY_LENGTH, length - 1);
}

void*
_DarrayPopAt(void* pArray, uint64_t index, void* pDest)
{
    uint64_t length = darray_length(pArray);
    uint64_t stride = darray_stride(pArray);
    if (index >= length)
	{
        YERROR("Index outside the bounds of this array! Length: %llu, index: %llu", length, index);
        return pArray;
    }
    uint64_t addr = (uint64_t)pArray;
    memcpy(pDest, (void*)(addr + (index * stride)), stride);

    // If not on the last element, snip out the entry and copy the rest inward.
    if (index != length - 1)
	{
        memcpy((void*)(addr + (index * stride)), (void*)(addr + ((index + 1) * stride)),
            stride * (length - index));
    }

    _DarrayFieldSet(pArray, DARRAY_LENGTH, length - 1);
    return pArray;
}

void*
_DarrayInsertAt(void* pArray, uint64_t index, void* pValue)
{
    uint64_t length = darray_length(pArray);
    uint64_t stride = darray_stride(pArray);
    if (index >= length)
	{
        YERROR("Index outside the bounds of this pArray! Length: %llu, index: %llu", length, index);
        return pArray;
    }
    if (length >= darray_capacity(pArray))
	{
    	pArray = _DarrayResize(pArray);
    }
    uint64_t addr = (uint64_t)pArray;
    // If not on the last element, copy the rest outward.
    if (index != length - 1)
	{
        memcpy( (void*)(addr + ((index + 1) * stride)), (void*)(addr + (index * stride)), stride * (length - index));
    }
    // Set the value at the index
    memcpy((void*)(addr + (index * stride)), pValue, stride);

    _DarrayFieldSet(pArray, DARRAY_LENGTH, length + 1);
    return pArray;
}
