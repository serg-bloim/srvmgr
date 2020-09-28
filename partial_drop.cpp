#include "utils.h"
#include "config_new.h"
#include <vector>
#define _USE_MATH_DEFINES
#include "math.h"

T_LINKEDLIST* __stdcall create_new_item_list()
{
	#define FUNC_ITEM_LIST_CONSTRUCTOR 0x00551C0A
	__asm
	{
		push	24h
		mov		edx, 0x005DDF54 // operator new
		call	edx
		add     esp, 4
		mov     ecx, eax
		mov		edx, FUNC_ITEM_LIST_CONSTRUCTOR
		call    edx
	}
}

float rnd(){
	return (float)rand() / (RAND_MAX + 1);
}

float rnd_gaussian(float mu, float sigma){
	float u1, u2;
	float two_pi = 2.0 * M_PI;
	u1 = rnd();
	u2 = rnd();
	float z0 = sqrt(-2.0 * log(u1)) * cos(two_pi * u2);
	return z0 * sigma + mu;
}

void insert(T_LINKEDLIST * list, T_SRV_LINKED_NODE* node){
	T_SRV_LINKED_NODE* first = list->first_node;
	node->next = first;
	list->first_node = node;
	node->prev = 0;
	if( first != 0){
		first->prev = node;
	}
	if(list->last_node == 0){
		list->last_node = node;
	}
	list->size++;
}

void a2insert(T_LINKEDLIST * list, int pos, T_INVENTORY_ITEM* item){
	__asm{
		mov		ecx, item
		push	ecx
		mov		ecx, pos
		push	ecx
		mov		ecx, list
		mov		edx, 0x00551FC3
		call	edx
	}
}
T_INVENTORY_ITEM* a2remove(T_LINKEDLIST * list, int pos, int n){
	__asm{
		mov		ecx, n
		push	ecx
		mov		ecx, pos
		push	ecx
		mov		ecx, list
		mov		edx, 0x00552E42
		call	edx
	}
}

void remove(T_LINKEDLIST * list, T_SRV_LINKED_NODE* node){
	if(list->first_node != nullptr){
		list->first_node->prev = node;
	}
	node->prev = nullptr;
	node->next=list->first_node;
	list->first_node = node;
	list->size++;
}
struct IndNum{
	__int16 ind;
	__int16 num;
};

float limit(float n, float lo, float hi){
	if(n < lo)
		return lo;
	if(n > hi)
		return hi;
	return n;
}
int round(float a){
	return (int)(a+0.5f);
}
int getDropNum(int num, float probability){
		int dropN = 0;
		if(probability == 0){
			dropN = 0;
		} else if(probability == 1){
			dropN = num;
		}else{
			if(num == 1){
				if(rnd() < probability){
					dropN = 1;
				}
			}else{
				float odds = limit(rnd_gaussian(probability, 0.1), 0, 1);
				dropN = round(odds * num);
			}
		}
		return dropN;
}
void __stdcall drop_rnd_items(T_LINKEDLIST * item_list_src, T_LINKEDLIST * item_list_dst, float probability)
{
	T_SRV_LINKED_NODE* src_current = item_list_src->last_node;
	int ind = item_list_src->size-1;
	std::vector<IndNum> to_remove;
	while(src_current != NULL){
		int dropN = getDropNum(src_current->value->amount, probability);
		if(dropN > 0){
			IndNum item;
			item.ind = ind;
			item.num = dropN;
			to_remove.push_back(item);
		}
		ind--;
		src_current = src_current->prev;
	}
	for (std::vector<IndNum>::iterator it = to_remove.begin() ; it != to_remove.end(); ++it){
		a2insert(item_list_dst, 0, a2remove(item_list_src, it->ind, it->num));
	}
}

bool __stdcall unit_has_weared_items(T_UNIT* unit)
{
	if(unit->weapon || unit->shield){
		return true;
	}
	for (int i = 1; i < 13; ++i )
	{
		T_INVENTORY_ITEM* item = (T_INVENTORY_ITEM*)((char*)unit+0x208)+i;
		if(item){
			return true;
		}
	}
	return false;
}
void __stdcall drop_rnd_weared_items(T_UNIT* unit, T_LINKEDLIST * item_list_dst, float probability)
{
	for (int i = 1; i < 13; ++i )
	{
		if(getDropNum(1, probability) > 0){
			__asm{
				mov		ecx, [i]
				mov		edx, [unit]
				mov		eax, [edx + ecx * 4 + 0x208]
				push	eax
				mov		ecx, [unit]			// this
				mov		edx, [ecx]
				mov		edx, [edx + 0x48]
				call	edx
				push	eax					// put item on stack
				mov		ecx,	item_list_dst
				mov		edx, 0x551FA3
				call	edx
			}
		}
	}
	// ugly asm code duplication. Need to refactor
	for (int i = 0; i < 2; ++i )
	{
		if(getDropNum(1, probability) > 0){
			__asm{
				mov		ecx, [i]
				mov		edx, [unit]
				mov		eax, [edx + ecx * 4 + 0x74]
				push	eax
				mov		ecx, [unit]			// this
				mov		edx, [ecx]
				mov		edx, [edx + 0x48]
				call	edx
				push	eax					// put item on stack
				mov		ecx,	item_list_dst
				mov		edx, 0x551FA3
				call	edx
			}
		}
	}
}

int CopyInventoryToMap(T_UNIT *unit, T_LINKEDLIST *inventory, int a3, int a4){
#define FUNC_COPY_INVENTORY_TO_MAP 0x0052D8D3
	__asm{
		mov		ecx, a4
		push	ecx
		mov		ecx, a3
		push	ecx
		mov		ecx, inventory
		push	ecx
		mov		ecx, unit	// this
		mov		edx, FUNC_COPY_INVENTORY_TO_MAP
		call    edx 
	}
}
const int T_UNIT_SKIP_DROP = 1;
const int T_UNIT_SKIP_DMG = 2;
bool nonStandardUnit(T_UNIT* unit, int spec){
	#define MAGIC_ITEM 0x1102
	if(unit && unit->inventory && unit->inventory->size >= 1){
		T_INVENTORY_ITEM* item = unit->inventory->first_node->value;
			if(item && item->id == MAGIC_ITEM){
				return item->amount & spec;
			}
	}
	return false;
}

T_LINKEDLIST* __stdcall drop_partially(T_UNIT* unit, int a3, int a4)
{
	if(unit && unit->inventory && (unit->inventory->size > 0 || unit_has_weared_items(unit))){
		if(nonStandardUnit(unit, T_UNIT_SKIP_DROP)){
			return 0;
		}
		T_LINKEDLIST* bag = create_new_item_list();
		drop_rnd_items(unit->inventory, bag, Config::InventoryDropPropapility);
		drop_rnd_weared_items(unit, bag, Config::WearDropPropapility);
		CopyInventoryToMap(unit, bag, a3, a4);
		return bag;
	}
	return 0;
}

void __declspec(naked) imp_drop_partially()
{ // 0052E264
    __asm
    {
		mov     ecx, [ebp-174h]		// pass a4
		push    ecx
		mov     edx, [ebp-1Ch]		// pass a3
		push    edx
		mov     ecx, [ebp-164h]		// pass unit
		push	ecx	
		call	drop_partially
		mov     [ebp-0C0h], eax		// pass ground bag

ret_point:
		ret		0xC
    }
}
int __stdcall check_unit_dmg(T_UNIT *unit, int a2, int a3){
	if(nonStandardUnit(unit, T_UNIT_SKIP_DMG)){
		return 0;
	}
	int res;
	__asm{
		mov		edx, a3
		push	edx
		mov		edx, a2
		push	edx
		mov		ecx, unit
		mov		edx, unit
		mov		edx, [edx]
		mov		edx, [edx + 0x54]
		call	edx
		mov		res, eax
	}
	return res;
}
int __declspec(naked) imp_check_unit_dmg()
{ // 0053693C
    __asm
    {
        mov     eax, [ebp+8]
        push    eax
        mov     ecx, [ebp-4]
        push    ecx
        mov     ecx, [ebp+0Ch]
		push	ecx
		call	check_unit_dmg
		mov     [ebp-0xC], eax
ret_point:
		ret		0x8
    }
}