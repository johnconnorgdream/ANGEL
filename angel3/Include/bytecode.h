#ifndef bytecode_def
#define bytecode_def
#ifdef _cplusplus
extern "c"{
#endif



#define bytecode_base_size 1000
#define dynamic_name_size 65535
#define regcount 5000  //先定义30个结果寄存器
//定义指令的表示
/*
算数逻辑指令
双目操作符
每个指令最多带两个操作数
*/


/*
运算指令
*/
#define _add_shared_local 0
#define _add_shared_shared 1
#define _add_local_local 2
#define _add_local_shared 3

#define _sub_shared_local 4
#define _sub_shared_shared 5
#define _sub_local_local 6
#define _sub_local_shared 7

#define _mult_shared_local 8
#define _mult_shared_shared 9
#define _mult_local_local 10
#define _mult_local_shared 11

#define _div_shared_local 12
#define _div_shared_shared 13
#define _div_local_local 14
#define _div_local_shared 15

#define _mod_shared_local 16
#define _mod_shared_shared 17
#define _mod_local_local 18
#define _mod_local_shared 19

#define _or_direct_local 20
#define _or_direct_shared 21
#define _or_direct_direct 22
#define _or_local_direct 23
#define _or_shared_direct 24
#define _or_shared_local 25
#define _or_shared_shared 26
#define _or_local_local 27
#define _or_local_shared 28

#define _and_direct_local 29
#define _and_direct_shared 30
#define _and_direct_direct 31
#define _and_local_direct 32
#define _and_shared_direct 33
#define _and_shared_local 34
#define _and_shared_shared 35
#define _and_local_local 36
#define _and_local_shared 37

#define _bitwise_or_shared_local 38
#define _bitwise_or_shared_shared 39
#define _bitwise_or_local_local 40
#define _bitwise_or_local_shared 41

#define _bitwise_and_shared_local 42
#define _bitwise_and_shared_shared 43
#define _bitwise_and_local_local 44
#define _bitwise_and_local_shared 45

#define _bitwise_xor_shared_local 46
#define _bitwise_xor_shared_shared 47
#define _bitwise_xor_local_local 48
#define _bitwise_xor_local_shared 49

#define _equal_shared_local 50
#define _equal_shared_shared 51
#define _equal_local_local 52
#define _equal_local_shared 53

#define _noequal_shared_local 54
#define _noequal_shared_shared 55
#define _noequal_local_local 56
#define _noequal_local_shared 57

#define _small_shared_local 58
#define _small_shared_shared 59
#define _small_local_local 60
#define _small_local_shared 61

#define _small_equal_shared_local 62
#define _small_equal_shared_shared 63
#define _small_equal_local_local 64
#define _small_equal_local_shared 65

#define _big_shared_local 66
#define _big_shared_shared 67
#define _big_local_local 68
#define _big_local_shared 69

#define _big_equal_shared_local 70
#define _big_equal_shared_shared 71
#define _big_equal_local_local 72
#define _big_equal_local_shared 73

#define _not_local 74
#define _not_shared 75
#define _not_direct 76

/*
存储指令
*/

#define _store_global_local 77
#define _store_global_shared 78
#define _store_global_temp 79

#define _store_local_local 80
#define _store_local_shared 81
#define _store_local_temp 82

#define _store_dynamic_local 83
#define _store_dynamic_shared 84
#define _store_dynamic_temp 85

#define _store_index_shared_local 86
#define _store_index_shared_shared 87
#define _store_index_shared_temp 88
#define _store_index_local_local 89
#define _store_index_local_shared 90
#define _store_index_local_temp 91

#define _store_static_local 92
#define _store_static_shared 93
#define _store_static_temp 94

#define _store_static_default_local 95
#define _store_static_default_shared 96
#define _store_static_default_temp 97

#define _store_member_shared_local 98
#define _store_member_shared_shared 99
#define _store_member_shared_temp 100
#define _store_member_local_local 101
#define _store_member_local_shared 102
#define _store_member_local_temp 103





//暂存到中间变量中，对于索引赋值这种两个以上参数的需要用该指令实现暂存
#define _mov_local 104
#define _mov_shared 105


/*
获取成员变量或索引取值
*/
#define _load_index_shared_local 106
#define _load_index_shared_shared 107
#define _load_index_local_local 108
#define _load_index_local_shared 109
#define _load_static 110
#define _load_static_default 111
#define _load_member_local 112
#define _load_member_shared 113
/*
控制指令
*/
#define _jnp 114
#define _jnp_bool_local 115
#define _jnp_bool_shared 116
#define _switch_case_local 117
#define _switch_case_shared 118
#define _jmp 119
#define _end 120
/*
函数调用
*/
#define _push_local 121
#define _push_shared 122
#define _call 123
#define _call_back 124
#define _call_member_local 125
#define _call_member_shared 126
#define _call_static 127
#define _call_static_default 128
#define _call_default 129
#define _sys_call 130
#define _ret 131
#define _ret_with_local 132
#define _ret_with_shared 133
#define _ret_obj 134
#define _nop 135 //空操作

/*
inplace操作
*/
#define _loadaddr_index_shared_local 136
#define _loadaddr_index_shared_shared 137
#define _loadaddr_index_local_local 138
#define _loadaddr_index_local_shared 139
#define _loadaddr_static 140
#define _loadaddr_static_default 141
#define _loadaddr_member_local 142
#define _loadaddr_member_shared 143

#define _inplace_add_global_local 144
#define _inplace_add_global_shared 145
#define _inplace_add_local_local 146
#define _inplace_add_local_shared 147

#define _inplace_sub_global_local 148
#define _inplace_sub_global_shared 149
#define _inplace_sub_local_local 150
#define _inplace_sub_local_shared 151

#define _inplace_mult_global_local 152
#define _inplace_mult_global_shared 153
#define _inplace_mult_local_local 154
#define _inplace_mult_local_shared 155

#define _inplace_div_global_local 156
#define _inplace_div_global_shared 157
#define _inplace_div_local_local 158
#define _inplace_div_local_shared 159

#define _inplace_mod_global_local 160
#define _inplace_mod_global_shared 161
#define _inplace_mod_local_local 162
#define _inplace_mod_local_shared 163

#define _inplace_bitwise_or_global_local 164
#define _inplace_bitwise_or_global_shared 165
#define _inplace_bitwise_or_local_local 166
#define _inplace_bitwise_or_local_shared 167

#define _inplace_bitwise_and_global_local 168
#define _inplace_bitwise_and_global_shared 169
#define _inplace_bitwise_and_local_local 170
#define _inplace_bitwise_and_local_shared 171

#define _inplace_bitwise_xor_global_local 172
#define _inplace_bitwise_xor_global_shared 173
#define _inplace_bitwise_xor_local_local 174
#define _inplace_bitwise_xor_local_shared 175
/*
其他指令,扩展指令
*/
#define _init_iter_local 176
#define _init_iter_shared 177
#define _iter 178
#define _is_item_shared_local 179
#define _is_item_shared_shared 180
#define _is_item_local_local 181
#define _is_item_local_shared 182
#define _load_dynamic 183
#define _bool_local 184
#define _bool_shared 185
#define _build_list 186
#define _append_list_local 187
#define _append_list_shared 188
#define _extend_list_local 189
#define _extend_list_shared 190
#define _build_set 191
#define _add_set_local 192
#define _add_set_shared 193 
#define _build_dict 194
#define _loadaddr_dynamic 195
#define _self_ladd_local 196
#define _self_ladd_shared 197
#define _self_radd_local 198
#define _self_radd_shared 199
#define _self_lsub_local 200
#define _self_lsub_shared 201
#define _self_rsub_local 202
#define _self_rsub_shared 203

#define _init_range_shared_local 204
#define _init_range_shared_shared 205
#define _init_range_local_local 206
#define _init_range_local_shared 207
#define _range_step 208
#define _ret_anyway 209
#define _init_class 210

#define _asc_ref 211
#define _dec_ref 212

#define _lshift_shared_local 213
#define _lshift_shared_shared 214
#define _lshift_local_local 215
#define _lshift_local_shared 216

#define _rshift_shared_local 217
#define _rshift_shared_shared 218
#define _rshift_local_local 219
#define _rshift_local_shared 220

#define _inplace_lshift_global_local 221
#define _inplace_lshift_global_global 222
#define _inplace_lshift_local_local 223
#define _inplace_lshift_local_global 224

#define _inplace_rshift_global_local 225
#define _inplace_rshift_global_global 226
#define _inplace_rshift_local_local 227
#define _inplace_rshift_local_global 228

#define _dynamic_get_function 229


typedef struct bytecodenode{
	char *code;
	unsigned long len,alloc_size;
}*bytecode;
bytecode initbytearray();
bytecode resize(bytecode bc);
void addbyte(bytecode bc,unsigned char c);
void insertbyte(bytecode bc,unsigned char c,int offset);
void copybyte(bytecode bc1,bytecode bc2);
void freebytecode(bytecode bc);
void addtwobyte(bytecode bc,uint16_t c);
void addfourbyte(bytecode bc,unsigned long c);



#ifdef _cplusplus
}
#endif
#endif