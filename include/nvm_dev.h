struct nvm_dev* nvm_dev_new(void);
void nvm_dev_free(struct nvm_dev *dev);

struct nvm_dev_info* nvm_dev_get_info(struct nvm_dev_info *dev);

struct nvm_dev_info* nvm_dev_info_new(void);
void nvm_dev_info_pr(struct nvm_dev_info *info);
void nvm_dev_info_free(struct nvm_dev_info **info);
int nvm_dev_info_fill(struct nvm_dev_info *info, const char *dev_name);

