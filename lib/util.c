extern char* baggy_size_table;

// can be optimized
void baggy_save_in_table(unsigned int loc, unsigned int allocation_size_lg) {
	for (int i = loc/16; i < loc/16 + (1<<(allocation_size_lg-4)); i++) {
		baggy_size_table[i] = (char) allocation_size_lg;
	}
}
