#include "stack.h"

const char * const err_msgs[] = {
    "OK", "ERR BAD ARG", "ERR CORRUPT", "ERR ALLOC"
};

const char* err_str(const err_t e)
{
    return err_msgs[e];
}



static size_t round_up(const size_t n, const size_t a)
{
    if (a == 0) return n;
    size_t r = n % a;
    return r ? (n + (a - r)) : n;
}

static inline void* calculate_ptr(const stack_t * const st, const size_t i)
{
    return (void*)((unsigned char*)st->data + i * st->elem_info.elem_stride);
}

static err_t stack_realloc(stack_t* st, const size_t new_capacity)
{
    err_t err = stack_verify(st);
    if(err != OK) return err;
    
    if (new_capacity == st->capacity) return OK;

    size_t new_bytes = new_capacity * st->elem_info.elem_stride;

    if (new_capacity == 0)
    {
        free(st->data);
        st->data     = NULL;
        st->capacity = 0;
        if (st->size > 0) st->size = 0;
        return OK;
    }

    void* p = (void*) realloc(st->data, new_bytes);
    if (!p) return ERR_ALLOC;

    if (new_capacity > st->capacity){
        const size_t old_bytes = st->capacity * st->elem_info.elem_stride;
        memset((unsigned char*)p + old_bytes, 0, new_bytes - old_bytes);
    }

    st->data     = p;
    st->capacity = new_capacity;
    if (st->size > st->capacity) st->size = st->capacity;

    return OK;
}

err_t stack_ctor(stack_t* st, element_info_t info,
                 const stack_print_fn printer, const stack_sprint_fn sprinter,
                 const stack_info_t stack_info)
{
    if(!CHECK(ERROR, st != NULL, "st == NULL on ctor"))              return ERR_BAD_ARG;
    if(!CHECK(ERROR, info.elem_size != 0, "elem_size == 0 on ctor")) return ERR_BAD_ARG;
    if(!CHECK(ERROR, printer != NULL, "printer == NULL on ctor"))    return ERR_BAD_ARG;

    if (info.elem_stride == 0)
    {
        info.elem_stride = round_up(info.elem_size, info.elem_align ? info.elem_align : 1);
    }

    st->stack_info = stack_info;
    st->elem_info  = info;

    st->data = NULL;
    st->size = 0;
    st->capacity = INITIAL_CAPACITY;

    st->printer  = printer;
    st->sprinter = sprinter;

    void* res = (void*) calloc(1, st->capacity * st->elem_info.elem_stride); 
    st->data  = res;

    return OK;
}

err_t stack_dtor(stack_t* st)
{
    if (st->data)
    {
        free(st->data);
        st->data = NULL;
    }
    st->size = 0;
    st->capacity = 0;

    return OK;
}

err_t stack_push(stack_t* st, const void* elem)
{
    STACK_VERIFY(st);

    if(!CHECK(ERROR, elem != NULL, "elem == NULL on push")) return ERR_BAD_ARG;

    err_t err;
    //err = stack_verify(st);
    //if(err != OK) return err;


    if (st->size == st->capacity){
        size_t target = st->capacity ? st->capacity * 2 : 4;
        err = stack_realloc(st, target);
        if (err != OK) return err;
    }
    memcpy(calculate_ptr(st, st->size), elem, st->elem_info.elem_size);
    st->size++;
    
    err = stack_verify(st);
    if(err != OK) return err;
    
    return OK;
}

err_t stack_pop (stack_t* st, void* elem)
{
    err_t err = stack_verify(st);
    if(err != OK) return err;
    if(!CHECK(ERROR, st->size > 0, "st size == 0 on pop")) return ERR_BAD_ARG;
    if(!CHECK(ERROR, elem != NULL, "elem == NULL on pop")) return ERR_BAD_ARG;

    memcpy(elem, calculate_ptr(st, st->size - 1), st->elem_info.elem_size);
    st->size--;

    err = stack_verify(st);
    if(err != OK) return err;
   
    if (st->capacity >= 8 && st->size <= st->capacity / 4){
        err = stack_realloc(st, st->capacity / 2);
        if(err != OK) return err;
    }
    return OK;
}

err_t stack_print(const stack_t* st)
{   
    err_t err = stack_verify(st);
    if(err != OK) return err;
    
    for (size_t i = 0; i < st->size; i++)
    {
        st->printer(stdout, calculate_ptr(st, i));
    }

    return OK;
}

static void print_hex_bytes(char* res_str, const void* p, const size_t n){
    const unsigned char* b = (const unsigned char*)p;
    size_t pos = strnlen(res_str, STR_CAT_MAX_SIZE);
    for (size_t i = 0; i < n && pos < STR_CAT_MAX_SIZE; ++i) {
        int written = snprintf(res_str + pos, STR_CAT_MAX_SIZE - pos, 
                               "%02X%s", b[i], (i + 1 < n ? " " : ""));
        if (written < 0) break;
        pos += written;
        if (pos >= STR_CAT_MAX_SIZE) { pos = STR_CAT_MAX_SIZE - 1; break; }
    }
}

err_t stack_dump (logging_level level, const stack_t* st, err_t code, const char* comment)
{
    char res_str[STR_CAT_MAX_SIZE] = {  };

    IFLOG(level, "=== STACK DUMP %p ===", (void*)st);

    snprintf(res_str, STR_CAT_MAX_SIZE, "status     : %s", err_str(code));
    if (comment) {
        size_t len = strnlen(res_str, STR_CAT_MAX_SIZE);
        snprintf(res_str + len, STR_CAT_MAX_SIZE - len, "  -- %s", comment);
    }

    IFLOG(level, res_str); 

    IFLOG(level, "name       : %s", st->stack_info.name ? st->stack_info.name : "(?)");
    
    IFDEBUG(
        IFLOG(level, "created    : %s:%d in %s",
            st->stack_info.file ? st->stack_info.file : "(?)",
            st->stack_info.line,
            st->stack_info.func ? st->stack_info.func : "(?)");
    )

    IFLOG(level, "type       : %s",  st->elem_info.elem_name ? st->elem_info.elem_name : "(?)");
    IFLOG(level, "elem size  : %zu", st->elem_info.elem_size);
    IFLOG(level, "elem align : %zu", st->elem_info.elem_align);
    IFLOG(level, "elem stride: %zu", st->elem_info.elem_stride);
    IFLOG(level, "data base  : %p",  (void*)st->data);

    IFLOG(level, "size/cap   : %zu / %zu", st->size, st->capacity);
    if (st->data == NULL) { IFLOG(level, "(no data) \n === END STACK DUMP ===\n"); return OK;  }

    for (size_t i = 0; i < st->capacity; i++)
    {
        const void* ptr = calculate_ptr(st, i);
        int used = (i < st->size);
        
        snprintf(res_str, STR_CAT_MAX_SIZE, " [%zu] %s", i, used ? "USED   " : "UNUSED ");

        size_t len2 = strnlen(res_str, STR_CAT_MAX_SIZE);
        if (st->data) print_hex_bytes(res_str, ptr, st->elem_info.elem_size);
        else          snprintf(res_str + len2, STR_CAT_MAX_SIZE - len2, "(no data)");

        if (used && st->printer){
            size_t len3 = strnlen(res_str, STR_CAT_MAX_SIZE);
            snprintf(res_str + len3, STR_CAT_MAX_SIZE - len2, "  | value: ");
            st->sprinter(res_str, STR_CAT_MAX_SIZE, ptr);
        }

        IFLOG(level, res_str);
    }

    IFLOG(level, "=== END STACK DUMP ===\n");

    return OK;
}

err_t stack_verify(const stack_t* st)
{
    STACK_CHECK(ERROR, st != NULL, st, ERR_BAD_ARG, "stack_verify: st is NULL");

    const element_info_t* ei = &st->elem_info;
    STACK_CHECK(ERROR, ei != NULL, st, ERR_CORRUPT, "stack_verify: elem info is NULL");

    STACK_CHECK(ERROR, ei->elem_size != 0, st, ERR_CORRUPT, "stack_verify: elem size == 0");
    
    STACK_CHECK(ERROR, ei->elem_align != 0, st, ERR_CORRUPT, "stack_verify: elem align == 0");

    STACK_CHECK(ERROR, ei->elem_stride >= ei->elem_size, st, ERR_CORRUPT, 
                "stack_verify: elem_stride (%zu) < elem_size (%zu)", ei->elem_stride, ei->elem_size);
    
    STACK_CHECK(ERROR, (ei->elem_stride % ei->elem_align) == 0, st, ERR_CORRUPT, 
                "stack_verify: elem_stride (%zu) %% elem_align (%zu) != 0", 
                ei->elem_stride, ei->elem_align);
    
    STACK_CHECK(ERROR, st->size <= st->capacity, st, ERR_CORRUPT, 
                "stack_verify: size (%zu) > capacity (%zu)", st->size, st->capacity);

    if (st->capacity == 0)
    {
        STACK_CHECK(ERROR, st->data == NULL, st, ERR_CORRUPT, 
                    "stack_verify: capacity == 0 but data != NULL (%p)", (void*)st->data);
    } else {
        STACK_CHECK(ERROR, st->data != NULL, st, ERR_CORRUPT, 
                    "stack_verify: capacity > 0 but data == NULL");
    }

    STACK_CHECK(ERROR, ((uintptr_t)st->data % ei->elem_align) == 0, st, ERR_CORRUPT,
                "stack_verify: base pointer %p is not aligned to %zu",
                (void*)st->data, ei->elem_align);

    const unsigned char* base = (const unsigned char*)st->data;
    const uintptr_t      ptr0 = (uintptr_t)(base + 0 * ei->elem_stride);

    STACK_CHECK(ERROR, (ptr0 % ei->elem_align) == 0, st, ERR_CORRUPT, 
                "stack_verify: ptr[0] misaligned: %p", (void*)ptr0);

    if (st->capacity > 1) {
        const uintptr_t ptr1 = (uintptr_t)(base + 1 * ei->elem_stride);
        STACK_CHECK(ERROR, (ptr1 % ei->elem_align) == 0, st, ERR_CORRUPT, 
                    "stack_verify: ptr[1] misaligned: %p", (void*)ptr1);
    }

    return OK;
}

