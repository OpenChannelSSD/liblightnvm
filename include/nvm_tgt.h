
typedef struct nvm_tgt_info *NVM_TGT_INFO;
typedef struct nvm_dev_info *NVM_DEV_INFO;

NVM_TGT_INFO nvm_tgt_info_new(void);
void nvm_tgt_info_free(NVM_TGT_INFO *info);
int nvm_tgt_info_fill(NVM_TGT_INFO info, const char *tgt_name);
void nvm_tgt_info_pr(NVM_TGT_INFO info);
