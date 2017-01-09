/*
gcc -Wl,--entry=func,--oformat=binary -nostdlib -fno-asynchronous-unwind-tables -m32 -o func3.o func3.c
ndisasm -b 32 func4.o

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEM_SIZE 0x100
#define STACK_SIZE 0x800

static int mnemonic_to_32bit(unsigned char *mnemonic);
static void get_mod_rm_reg(unsigned char , unsigned char *,unsigned char *, unsigned char *);
static unsigned char get_disp_length(unsigned char , unsigned char );
static void print_out_registor(void);

typedef struct {
	int eax;
	int ecx;
	int edx;
	int ebx;
	int esp;
	int ebp;
	int esi;
	int edi;
} CPU_GE_REGISTOR;

typedef struct {
	int eip;
	short eflags;
	//bit flag 
	//0 CF add/sub で繰り上がり下がりが発生したら１ 1
	//6 ZF 演算結果が０なら１ 32
	//7 SF 符号ビット。最上位ビットが１なら１ 64
	//11 OF オーバーフローフラグ 1024
} CPU_SP_RGISTER;

typedef struct {
	CPU_GE_REGISTOR g_reg;
	CPU_SP_RGISTER sp_reg;
}CPU_REG;

CPU_REG cpu_reg = {0};
unsigned char main_memory_stack[STACK_SIZE] = {0};
unsigned char main_memory[MEM_SIZE] = {0};
int stack_init_point = 0;


/* immediate codeのバイト数を返す。byte */
static unsigned char get_imme_length(unsigned char code){
	if (code == 0x83) {
		return 1;
	}
}

/*メインメモリから命令を取り出す*/
static unsigned char fetch(unsigned char *mnemonic){
	unsigned char code = 0;
	unsigned char length = 0;
	unsigned char MOD;
	unsigned char RM;
	unsigned char REG;
	unsigned char disp_len;

	int eip = cpu_reg.sp_reg.eip;

	/*1byte取り出し*/
	code = *(main_memory + eip);
	length++;//1byte取り出し分

	//modrmとdispをとりだしを取り出し。
	if (code == 0x89 ||
	    code == 0x8b){
		get_mod_rm_reg(*(main_memory + eip + 1), &MOD, &RM, &REG);//+1はRM/MOD
		length++;//modrm分
		disp_len = get_disp_length(MOD, RM);
		length = length + disp_len;//disp_len分
	} else if (code == 0x83){
		get_mod_rm_reg(*(main_memory + eip + 1), &MOD, &RM, &REG);//+1はRM/MOD
		length++;//modrm分
		disp_len = get_disp_length(MOD, RM);
		length = length + disp_len;//disp_len分

		/*immediate code length*/
		length = length + get_imme_length(code);
	} else if (code == 0x90) {
		length = 1;
	} else if (code == 0x55) {
		length = 1;
	} else if (code == 0x48) {
		length = 1;
	} else if (code == 0x5D) {
		length = 1;
	} else if (code == 0xC3) {
		length = 1;
	} else if (code == 0xE8) {
		length = 5;
	} else if (code == 0xb8) {
		length = 5;
	} else if (code == 0xbe) {
		length = 5;
	} else if (code == 0xbf) {
		length = 5;
	} else if (code == 0x01) {
		length = 2;
	} else if (code == 0xc9) {
		length = 1;
	} else if (code == 0x53) {
		length = 1;
	} else if (code == 0x5b) {
		length = 1;
	} else if (code == 0x50) {
		length = 1;
	} else if (code == 0x6a) {
		length = 2;
	} else if (code == 0x85) {
		length = 2;
	} else if (code == 0x74) {
		length = 2;
	} else if (code == 0xeb) {
		length = 2;
	}

	/* メインメモリーからプログラムを取り出し */
	memcpy(mnemonic, main_memory + eip, length);
	
	/*eipを移動*/
	cpu_reg.sp_reg.eip = cpu_reg.sp_reg.eip + length;

	return length;
}

static int *get_register_from_MOD_RM(unsigned char *mnemonic, unsigned char MOD, unsigned char RM)
{
	//displasementがあれば足す
	char disp_8bit = 0;
	int disp_32bit = 0;

	int disp_len = get_disp_length(MOD, RM);
//	printf("%d\n", disp_len);

	if(disp_len == 1){
		memcpy(&disp_8bit, mnemonic + 2, 1);//1 1byte
	} else if (disp_len == 4){
		memcpy(&disp_32bit, mnemonic + 2, 4);//4 1byte
		//エンディアン変換がいるかも 
	}
//	printf("disp32 = %d\n", disp_32bit);
//	printf("disp32 = 0x%x\n", disp_32bit);
//	printf("disp8 = %d\n", disp_8bit);
//	printf("disp8 = 0x%x\n", disp_8bit);

	int *ret = NULL;

	/*
	 * Mod
	 * 00 レジスタに入ってるアドレスにかいこむ
	 * 01 アドレス＋disp8
	 * 10 アドレス＋disp32
	 * 11 レジスタに書き込む
	 */
	if (MOD == 0) {//00
		switch (RM) {
			case 0x0:
				ret = (int*)cpu_reg.g_reg.eax;
				break;
			case 0x1://001
				ret = (int*)cpu_reg.g_reg.ecx;
				break;
			case 0x2://010
				ret = (int*)cpu_reg.g_reg.edx;
				break;
			case 0x3://011
				ret = (int*)cpu_reg.g_reg.ebx;
				break;
			case 0x4://100
				printf("niy %d\n", __LINE__);
				break;
			case 0x5://101
				ret = (int*)disp_32bit;
				break;
			case 0x6://110
				ret = (int*)cpu_reg.g_reg.esi;
				break;
			case 0x7://111                
				ret = (int*)cpu_reg.g_reg.edi;
				break;
			default:
				printf("error\n");
				break;
		}
	}  else if (MOD == 1) {//01
		switch (RM) {
			case 0x0://000
				ret = (int*)(cpu_reg.g_reg.eax + disp_8bit);
				break;
			case 0x1://001
				ret = (int*)(cpu_reg.g_reg.ecx + disp_8bit);
				break;
			case 0x2://010
				ret = (int*)(cpu_reg.g_reg.edx + disp_8bit);
				break;
			case 0x3://011
				ret = (int*)(cpu_reg.g_reg.ebx + disp_8bit);
				break;
			case 0x4://100
				/*[-][-]+disp32 */
				break;
			case 0x5://101
				ret = (int*)(cpu_reg.g_reg.ebp + disp_8bit);
				break;
			case 0x6://110
				ret = (int*)(cpu_reg.g_reg.esi + disp_8bit);
				break;
			case 0x7://111
				ret = (int*)(cpu_reg.g_reg.edi + disp_8bit);
				break;
			default:
				printf("error\n");
				break;
		}
	} else if (MOD == 2) {//010
		switch (RM) {
			case 0x0:
				ret = (int*)(cpu_reg.g_reg.eax + disp_32bit);
				break;
			case 0x1:
				ret = (int*)(cpu_reg.g_reg.ecx + disp_32bit);
				break;
			case 0x2:
				ret = (int*)(cpu_reg.g_reg.edx + disp_32bit);
				break;
			case 0x3:
				ret = (int*)(cpu_reg.g_reg.ebx + disp_32bit);
				break;
			case 0x4:
				/* [-][-]+disp32 */
				break;
			case 0x5:
				ret = (int*)(cpu_reg.g_reg.ebp + disp_32bit);
				break;
			case 0x6:
				ret = (int*)(cpu_reg.g_reg.esi + disp_32bit);
				break;
			case 0x7:                     
				ret = (int*)(cpu_reg.g_reg.edi + disp_32bit);
				break;
			default:
				printf("error\n");
				break;
		}
	} else if(MOD == 3){//11
		switch (RM) {
			case 0x0:
				ret = &cpu_reg.g_reg.eax;
				break;
			case 0x1:
				ret = &cpu_reg.g_reg.ecx;
				break;
			case 0x2:
				ret = &cpu_reg.g_reg.edx;
				break;
			case 0x3:
				ret = &cpu_reg.g_reg.ebx;
				break;
			case 0x4:
				ret = &cpu_reg.g_reg.esp;
				break;
			case 0x5:
				ret = &cpu_reg.g_reg.ebp;
				break;
			case 0x6:
				ret = &cpu_reg.g_reg.esi;
				break;
			case 0x7:                     
				ret = &cpu_reg.g_reg.edi;
				break;
			default:
				printf("error\n");
				break;
		}
	}
	printf("get_register_from_MOD_RM ret %x\n", (int*)ret);
	return ret;
}

//REGからレジスターの中に入っている値を取得する関数
static int get_register_from_REG(unsigned char REG){
	int ret = 0;
	switch (REG) {
		case 0x0:
			ret = cpu_reg.g_reg.eax;
			break;
		case 0x1:
			ret = cpu_reg.g_reg.ecx;
			break;
		case 0x2:
			ret = cpu_reg.g_reg.edx;
			break;
		case 0x3:
			ret = cpu_reg.g_reg.ebx;
			break;
		case 0x4:
			ret = cpu_reg.g_reg.esp;
			break;
		case 0x5:
			ret = cpu_reg.g_reg.ebp;
			break;
		case 0x6:
			ret = cpu_reg.g_reg.esi;
			break;
		case 0x7:                     
			ret = cpu_reg.g_reg.edi;
			break;
		default:
			printf("error\n");
			break;
	}
	printf("get_register_from_REG ret = %d, REG = 0x%x\n", ret, REG);
	return ret;
}


//REGで指定されるレジスターのアドレスを返す
static int *get_register_pointer_from_REG(unsigned char REG){
	int *ret = 0;
	switch (REG) {
		case 0x0:
			ret = &cpu_reg.g_reg.eax;
			break;
		case 0x1:
			ret = &cpu_reg.g_reg.ecx;
			break;
		case 0x2:
			ret = &cpu_reg.g_reg.edx;
			break;
		case 0x3:
			ret = &cpu_reg.g_reg.ebx;
			break;
		case 0x4:
			ret = &cpu_reg.g_reg.esp;
			break;
		case 0x5:
			ret = &cpu_reg.g_reg.ebp;
			break;
		case 0x6:
			ret = &cpu_reg.g_reg.esi;
			break;
		case 0x7:                     
			ret = &cpu_reg.g_reg.edi;
			break;
		default:
			printf("error\n");
			break;
	}
	printf("get_register_from_REG ret = %p, REG = 0x%x\n", ret, REG);
	return ret;
}



static void get_mod_rm_reg(unsigned char mod_opeland, unsigned char *MOD,
			unsigned char *RM, unsigned char *REG) {
	*MOD = mod_opeland >> 6;
	unsigned char RM_tmp = mod_opeland << 5;
	*RM = RM_tmp >> 5;
	unsigned char REG_mod = mod_opeland << 2;
	*REG = REG_mod >> 5;

//	printf("MOD=%d\n", *MOD);
//	printf("RM=%d\n", *RM);
//	printf("REG=%d\n", *REG);
}
static unsigned char get_disp_length(unsigned char MOD, unsigned char RM){
	char ret = 0;
	switch (MOD){
	case 0x10:
		ret = 4;	
		break;
	case 0x01:
		ret = 1;	
		break;
	case 0x00:
		if (RM == 0x101){
			ret = 4;	
		}
		break;
	default:
		break;
	}

	printf("get_disp_length%d\n", ret);
	return ret;

}

/*命令を解読する*/
static void execute(unsigned char *mnemonic) {
	unsigned char MOD;
	unsigned char RM;
	unsigned char REG;
	int *reg_register1;
	int *reg_register2;
	char displasement_8bit;
	int displasement_32bit;
	unsigned char disp_len;
	int tmp_32bit = 0;
	unsigned char tmp_1byte;

	if (*mnemonic == 0x89) {
		printf("mov 89\n");
		//opecode1byte modrm1byte disp VARbyte
		get_mod_rm_reg(*(mnemonic+1), &MOD, &RM, &REG);

		//REGは上書く側のレジスタ(後ろ) 足しこむ奴が入ってるやつの値
		reg_register2 = get_register_from_REG(REG);

		//get_register_from_MODで上書きする場所を取得
		reg_register1 = get_register_from_MOD_RM(mnemonic, MOD, RM);

		/* 足しこむ場所のアドレスがわかったので */
		printf("reg_register1 %d\n", reg_register1);
		printf("reg_register2 %d\n", reg_register2);
		*reg_register1 = reg_register2;
	} else if (*mnemonic == 0x83) {
		printf("add or sub 83\n");
		//opecode1byte modrm1byte disp VARbyte
		int immediate = 0;
		get_mod_rm_reg(*(mnemonic+1), &MOD, &RM, &REG);
		if(MOD == 1){//0x01
			immediate = (char)*(mnemonic+3);
		} else if (MOD == 2){//0x10
			immediate = (char)*(mnemonic+6);
		} else if (MOD == 3){//0x11
			immediate = (char)*(mnemonic+2);
		}
		printf("immediate(%d)\n", immediate);
		//get_register_from_MODで上書きする場所を取得
		reg_register1 = get_register_from_MOD_RM(mnemonic, MOD, RM);
		printf("reg_register1 %x\n", reg_register1);

		//フラグの設定
		cpu_reg.sp_reg.eflags = 0;
		printf("reg1(%d),imm(%d) eflags(0x%x)\n",
			*reg_register1,immediate, cpu_reg.sp_reg.eflags);
		if (*reg_register1 == immediate) {
			cpu_reg.sp_reg.eflags |= 0x20;//32
		}
		if (*reg_register1 < immediate) {
			cpu_reg.sp_reg.eflags |= 0x40;//64
		}
		if (*reg_register1 == -2147483648 && immediate) {
			cpu_reg.sp_reg.eflags |= 0x400;//1024
		}

		//CFがどういう時に設定されるべきなのかよくわからないので実装していない。
		printf("reg1(%d),imm(%d) eflags(0x%x)\n",
			*reg_register1,immediate, cpu_reg.sp_reg.eflags);
		if (REG == 0) {
			//add
			//immediate codeを書き込み
			printf("add REG(%d)\n", REG);
			*reg_register1  = *reg_register1 + immediate;
		} else if (REG == 5) {
			//sub
			//REG == 5 sub
			printf("sub REG(%d)\n", REG);
			*reg_register1  = *reg_register1 - immediate;
		} else if(REG == 7){
		printf("cmp REG(%d)\n", REG);
			//cmp nothing to do 
		} else {
			printf("0x83 error no match REG(%d)\n",REG);
			exit(0);
		}
		printf("%p %d\n", reg_register1, *reg_register1);
	} else if (*mnemonic == 0x90) {
		printf("nop 90\n");
	} else if (*mnemonic == 0x55) {
		printf("55 push ebp\n");
		cpu_reg.g_reg.esp -= sizeof(int);
		(*(int*)(cpu_reg.g_reg.esp)) = cpu_reg.g_reg.ebp;
		printf("STACK PUSH!! ebp%d\n", cpu_reg.g_reg.ebp);
	} else if (*mnemonic == 0x50) {
		printf("50 push eax\n");
		cpu_reg.g_reg.esp -= sizeof(int);
		(*(int*)(cpu_reg.g_reg.esp)) = cpu_reg.g_reg.eax;
		printf("STACK PUSH!! eax%d\n", cpu_reg.g_reg.eax);
	} else if (*mnemonic == 0x53) {
		printf("53 push ebx\n");
		cpu_reg.g_reg.esp -= sizeof(int);
		(*(int*)(cpu_reg.g_reg.esp)) = cpu_reg.g_reg.ebx;
		printf("STACK PUSH!! ebx%d\n", cpu_reg.g_reg.ebx);

	} else if (*mnemonic == 0x6a) {
		printf("6a push imm\n");
		int immediate = 0;
		cpu_reg.g_reg.esp -= sizeof(int);
		immediate =  (char)*(mnemonic+1);
		(*(int*)(cpu_reg.g_reg.esp)) = immediate;
		printf("STACK PUSH!! imm%d\n", immediate);

	} else if (*mnemonic == 0x48) {
		printf("48 dec eax\n");
		cpu_reg.g_reg.eax--;

	} else if (*mnemonic == 0x5D) {
		printf("pop 5D\n");
		cpu_reg.g_reg.ebp = *(int*)(cpu_reg.g_reg.esp);
		cpu_reg.g_reg.esp += sizeof(int);
		printf("STACK POP ebp!! %d\n", cpu_reg.g_reg.ebp);

	} else if (*mnemonic == 0x5b) {
		printf("pop 5B\n");
		cpu_reg.g_reg.ebx = *(int*)(cpu_reg.g_reg.esp);
		cpu_reg.g_reg.esp += sizeof(int);
		printf("STACK POP ebx!! %d\n", cpu_reg.g_reg.ebx);


	} else if (*mnemonic == 0xC3) {
		printf("ret C3\n");
		cpu_reg.sp_reg.eip = *(int*)(cpu_reg.g_reg.esp);
		cpu_reg.g_reg.esp += sizeof(int);
		printf("STACK POP on ret!! %d\n", cpu_reg.sp_reg.eip);

	} else if (*mnemonic == 0xE8) {
		printf("call E8\n");
		cpu_reg.g_reg.esp -= sizeof(int);
		*(int*)(cpu_reg.g_reg.esp) = cpu_reg.sp_reg.eip;
		printf("STACK PUSH address !! %d\n", *(int*)(cpu_reg.g_reg.esp));

		printf("call E8\n");
		printf("[%x]", *(mnemonic));
		printf("[%x]", *(mnemonic+1));
		printf("[%x]", *(mnemonic+2));
		printf("[%x]", *(mnemonic+3));
		printf("[%x]\n", *(mnemonic+4));

		tmp_1byte = *(mnemonic+4);
		tmp_32bit |= tmp_1byte;
		tmp_32bit = tmp_32bit << 8;
		tmp_1byte = *(mnemonic+3);
		tmp_32bit |= tmp_1byte;
		tmp_32bit = tmp_32bit << 8;
		tmp_1byte = *(mnemonic+2);
		tmp_32bit |= tmp_1byte;
		tmp_32bit = tmp_32bit << 8;
		tmp_1byte = *(mnemonic+1);
		tmp_32bit |= tmp_1byte;

		printf("tmp_32bit%d\n", tmp_32bit);
		printf("tmp_32bit%x\n", tmp_32bit);
		printf("cpu_reg.sp_reg.eip%d\n", cpu_reg.sp_reg.eip);
		printf("%x\n");

		/*eipはfetchした瞬間増えてるので、
		 *本来であれば次の命令のアドレスである+5をしてから引き算するのだが、
		 *ここでは不要となる。
		*/
		cpu_reg.sp_reg.eip = cpu_reg.sp_reg.eip + tmp_32bit;
	} else if (*mnemonic == 0xb8) {
		printf("b8 mov eax xxx\n");
		printf("[%x]", *(mnemonic));
		cpu_reg.g_reg.eax = mnemonic_to_32bit(mnemonic);;
	} else if (*mnemonic == 0xbe) {
		printf("be mov esi xxx\n");
		printf("[%x]", *(mnemonic));
		cpu_reg.g_reg.esi = mnemonic_to_32bit(mnemonic);
	} else if (*mnemonic == 0xbf) {
		printf("bf mov edi xxx\n");
		printf("[%x]", *(mnemonic));
		cpu_reg.g_reg.edi = mnemonic_to_32bit(mnemonic);

	} else if (*mnemonic == 0x8b) {
		printf("mov 8b\n");
		get_mod_rm_reg(*(mnemonic+1), &MOD, &RM, &REG);

		reg_register1 = get_register_pointer_from_REG(REG);
		reg_register2 = get_register_from_MOD_RM(mnemonic, MOD, RM);

		printf("reg_register1 %d\n", *reg_register1);
		printf("reg_register2 %d\n", *reg_register2);
		*reg_register1 = *reg_register2;
	} else if (*mnemonic == 0x01) {
		printf("add 01\n");
		get_mod_rm_reg(*(mnemonic+1), &MOD, &RM, &REG);

		reg_register2 = get_register_from_MOD_RM(mnemonic, MOD, RM);
		//eax = eax + edx
		*reg_register2 = *reg_register2 + get_register_from_REG(REG);

		printf("reg_register2 %d\n", *reg_register2);

	} else if (*mnemonic == 0xc9) {
		printf("c9 mov esp ebp\n");
		cpu_reg.g_reg.esp = cpu_reg.g_reg.ebp;
		printf("moved ebp(%d) ebp(%d)\n", cpu_reg.g_reg.esp, cpu_reg.g_reg.ebp);

		printf("c9 pop ebp\n");
		cpu_reg.g_reg.ebp = *(int*)(cpu_reg.g_reg.esp);
		cpu_reg.g_reg.esp += sizeof(int);
		printf("poped ebp(%d) ebp(%d)\n", cpu_reg.g_reg.esp, cpu_reg.g_reg.ebp);
	} else if (*mnemonic == 0x85) {
		printf("85 test\n");
		get_mod_rm_reg(*(mnemonic+1), &MOD, &RM, &REG);
		cpu_reg.sp_reg.eflags = 0;
		reg_register1 = get_register_from_MOD_RM(mnemonic, MOD, RM);
		// test rm , REG
		if((*reg_register1 & get_register_from_REG(REG)) == 0){
			cpu_reg.sp_reg.eflags = 32;
		}
		printf("reg_register1%d REG%d\n", *reg_register1, get_register_from_REG(REG));
		printf("cpu_reg.sp_reg.eflags %d\n", cpu_reg.sp_reg.eflags);

	} else if (*mnemonic == 0x74) {
		printf("eflags%d eip%d\n", cpu_reg.sp_reg.eflags, cpu_reg.sp_reg.eip);
		if(cpu_reg.sp_reg.eflags == 32){
			cpu_reg.sp_reg.eip += (char)*(mnemonic+1);
			printf("eip%d\n", cpu_reg.sp_reg.eip);
		}
	} else if (*mnemonic == 0xeb) {
		cpu_reg.sp_reg.eip += (char)*(mnemonic+1);
		printf("eip,%d->%d\n", cpu_reg.sp_reg.eip, (char)*(mnemonic+1));
	} else {
		printf("unknown opecode [%x]\n", *mnemonic);
		exit(0);
	}
}

static int mnemonic_to_32bit(unsigned char *mnemonic){
	int tmp32 = 0;
	int tmp1byte = 0;
	printf("[%x]", *(mnemonic+1));
	printf("[%x]", *(mnemonic+2));
	printf("[%x]", *(mnemonic+3));
	printf("[%x]\n", *(mnemonic+4));

	tmp1byte = *(mnemonic+4);
	tmp32 |= tmp1byte;
	tmp32 = tmp32 << 8;
	tmp1byte = *(mnemonic+3);
	tmp32 |= tmp1byte;
	tmp32 = tmp32 << 8;
	tmp1byte = *(mnemonic+2);
	tmp32 |= tmp1byte;
	tmp32 = tmp32 << 8;
	tmp1byte = *(mnemonic+1);
	tmp32 |= tmp1byte;

	printf("tmp32%d\n", tmp32);
	return tmp32;
}


static void print_mnemonic(unsigned char *mn){
	int i;
	printf("mne=");
	for(i = 0; i < 15; i){
		printf("%x,", mn[i]);
		i++;
	}
	printf("\n");
}

/*実行*/
static void simulator_run(int start_point) {
	/* ニーモニックの箱 */
	unsigned char mnemonic[15] = {0};
	cpu_reg.sp_reg.eip = start_point;
	int i;
	int length;
	print_out_registor();
	for (i = 0;;) {
//		getchar();
		memset(mnemonic, 0, 15);
		length = fetch(mnemonic);
		printf("length%d\n", length);
		print_mnemonic(mnemonic);
		execute(mnemonic);
		print_out_registor();
		printf("%x %x\n",stack_init_point, cpu_reg.g_reg.esp);
		if(stack_init_point == (int*)cpu_reg.g_reg.esp) {
			printf("end\n");
			exit(0);
		}
		if(cpu_reg.sp_reg.eip == 0x6a) {
			printf("2end\n");
			exit(0);
		}
	}
}

static long get_file_size(FILE *fp){
	long ret;
	fseek(fp, 0, SEEK_END);
	ret = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	return ret;
}

static void print_out_registor(){
	printf("%p eax(%d)\t\t",&cpu_reg.g_reg.eax,cpu_reg.g_reg.eax);
	printf("%p ecx(%d)\n",&cpu_reg.g_reg.ecx,cpu_reg.g_reg.ecx);
	printf("%p edx(%d)\t\t",&cpu_reg.g_reg.edx,cpu_reg.g_reg.edx);
	printf("%p ebx(%d)\n",&cpu_reg.g_reg.ebx,cpu_reg.g_reg.ebx);
	printf("%p esp(%d)\t",&cpu_reg.g_reg.esp,cpu_reg.g_reg.esp);
	printf("%p esp(0x%x)\n",&cpu_reg.g_reg.esp,cpu_reg.g_reg.esp);
	printf("%p ebp(%d)\t",&cpu_reg.g_reg.ebp,cpu_reg.g_reg.ebp);
	printf("%p ebp(0x%x)\n",&cpu_reg.g_reg.ebp,cpu_reg.g_reg.ebp);
	printf("%p esi(%d)\t\t",&cpu_reg.g_reg.esi,cpu_reg.g_reg.esi);
	printf("%p edi(%d)\n",&cpu_reg.g_reg.edi,cpu_reg.g_reg.edi);
	printf("%p eip(0x%x)\t",&cpu_reg.sp_reg.eip,cpu_reg.sp_reg.eip);
	printf("%p eflags(%d)\n",&cpu_reg.sp_reg.eflags,cpu_reg.sp_reg.eflags);
	int i;
	for(i = 0;(int*)(cpu_reg.g_reg.esp + i * sizeof(int)) != stack_init_point;i++) {
		printf("[%x]", *((int*)(cpu_reg.g_reg.esp + i * sizeof(int))));
	}
	printf("\n");
	printf("\n----\n");
}

int main() {
	/*espを適当なところに設定
	グローバル領域で代入できないのでしかたなくここでやる*/
	cpu_reg.g_reg.esp = &main_memory_stack[2048];
	stack_init_point = (int*)cpu_reg.g_reg.esp;

	FILE *fp;
	fp = fopen("fibo.o","rb");

	long file_size = get_file_size(fp);
	
	if (MEM_SIZE < file_size){
		printf("file size over\n");
		exit(0);
	}

	fread(main_memory,file_size,1,fp);
	int i;
	for(i = 0; i < file_size;i++){
		printf("%x,", main_memory[i]);
	}
	printf("\n");
	fclose(fp);

	simulator_run(0x50);

	return 0;
}

