#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void mod_rm_reg(unsigned char , unsigned char *,unsigned char *, unsigned char *);
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
	int eflags;
} CPU_SP_RGISTER;

typedef struct {
	CPU_GE_REGISTOR g_reg;
	CPU_SP_RGISTER sp_reg;
}CPU_REG;

typedef struct {
	unsigned char code;//ex.mov 89
	unsigned char length;//byte
}_OPE_CODE;

CPU_REG cpu_reg = {0};

_OPE_CODE ope_code[] = {
	{0x89, 1},//2コメはrmmodだと想定。。。
	{0x83, 1}, 
};

unsigned char main_memory[8] = {0};

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
	if (code == 0x89){
		mod_rm_reg(*(main_memory + eip + 1), &MOD, &RM, &REG);//+1はRM/MOD
		length++;//modrm分
		disp_len = get_disp_length(MOD, RM);
		length = length + disp_len;//disp_len分
	} else if (code == 0x83){
		
	}
	//一気に移動したらエンディアン変換が必要かも。
	memcpy(mnemonic, main_memory + eip, length);
	
	/*
	 * デバッグプリント
	 */
	int i;
	for(i = 0; i < 15;i++){
		printf("fetch %x\n", mnemonic[i]);
	}

	/*eipを移動*/
	cpu_reg.sp_reg.eip = cpu_reg.sp_reg.eip + length;

	return length;
}

static int *get_register_from_MOD_RM(unsigned char *mnemonic, unsigned char MOD, unsigned char RM)
{
	//displasementがあれば足す
	int disp_8bit = 0;
	int disp_32bit = 0;

	int disp_len = get_disp_length(MOD, RM);
	printf("%d\n", disp_len);

	if(disp_len == 1){
		memcpy(&disp_8bit, mnemonic + 2, 1);//1 1byte
	} else if (disp_len == 4){
		memcpy(&disp_32bit, mnemonic + 2, 4);//4 1byte
		//エンディアン変換がいるかも 
	}
	printf("disp32 = %d\n", disp_32bit);
	printf("disp8 = %d\n", disp_8bit);

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
			case 0x4:
				ret = &cpu_reg.g_reg.ebx;
				break;
			case 0x3:
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


static int* get_register_from_REG(unsigned char REG){
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
		case 0x4:
			ret = &cpu_reg.g_reg.ebx;
			break;
		case 0x3:
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
	return ret;
}



static void mod_rm_reg(unsigned char mod_opeland, unsigned char *MOD,
			unsigned char *RM, unsigned char *REG) {
	*MOD = mod_opeland >> 6;
	unsigned char RM_tmp = mod_opeland << 5;
	*RM = RM_tmp >> 5;
	unsigned char REG_mod = mod_opeland << 2;
	*REG = REG_mod >> 5;

	printf("MOD=%d\n", *MOD);
	printf("RM=%d\n", *RM);
	printf("REG=%d\n", *REG);
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

	if (*mnemonic == 0x89) {
		//opecode1byte modrm1byte disp VARbyte
		mod_rm_reg(*(mnemonic+1), &MOD, &RM, &REG);

		//REGは上書く側のレジスタ(後ろ) 足しこむ奴が入ってるやつのアドレス
		reg_register2 = get_register_from_REG(REG);
		printf("reg_register2 addr %x\n", reg_register2);

		//get_register_from_MODで上書きする場所を取得
		reg_register1 = get_register_from_MOD_RM(mnemonic, MOD, RM);

		/* 足しこむ場所のアドレスがわかったので */
		printf("reg_register1 %d\n", reg_register1);
		printf("reg_register2 %d\n", reg_register2);
		*reg_register1 = *reg_register2;
	}
}

/*実行*/
static void simulator_run() {
	/* ニーモニックの箱 */
	unsigned char mnemonic[15] = {0};
	unsigned char ope_length = 0;

	memset(mnemonic, 0, 15);

	/* フェッチ */
	fetch(mnemonic);
	int i;
	for(i = 0; i < 15; i){
		printf("mnemonic=%x\n", mnemonic[i]);
		i++;
	}
	
	/* デコードできずww */

	/* デコード? */
	execute(mnemonic);


	print_out_registor();

	/* 次の命令*/
//	memset(mnemonic, 0, 15);
//	ope_length = fetch(mnemonic);

}

static void print_out_registor(){
	printf("eax(%d)\n",cpu_reg.g_reg.eax);
	printf("ecx(%d)\n",cpu_reg.g_reg.ecx);
	printf("edx(%d)\n",cpu_reg.g_reg.edx);
	printf("ebx(%d)\n",cpu_reg.g_reg.ebx);
	printf("esp(%d)\n",cpu_reg.g_reg.esp);
	printf("ebp(%d)\n",cpu_reg.g_reg.ebp);
	printf("esi(%d)\n",cpu_reg.g_reg.esi);
	printf("edi(%d)\n",cpu_reg.g_reg.edi);
	printf("eip(%d)\n",cpu_reg.sp_reg.eip);
	printf("eflags(%d)\n",cpu_reg.sp_reg.eflags);
}

int main() {
	FILE *fp;
	fp = fopen("func_sample.bin","rb");

	fread(main_memory,8,1,fp);
	int i = 0;
	for(i = 0; i < 8;i++){
		printf("%x\n", main_memory[i]);
	}
	fclose(fp);

	simulator_run();


	return 0;
}


