#include "stdafx.h"
#include "pe_import.h"


imported_func::imported_func() {
	hint				= 0;
	ordinal				= 0;
	b_import_by_name	= 0;
	iat_rva				= 0;
	name.clear();
}
imported_func::imported_func(DWORD iat_rva, std::string name, WORD ordinal, WORD hint) {
	this->iat_rva = iat_rva;
	this->hint = hint;
	this->name = name;
	this->ordinal = ordinal;

	if (this->name.length()) {
		b_import_by_name = true;
	}
	else {
		b_import_by_name = false;
	}

}


imported_func::~imported_func() {

}

imported_func& imported_func::operator=(const imported_func& func) {

	this->iat_rva = func.iat_rva;
	this->hint = func.hint;
	this->name = func.name;
	this->ordinal = func.ordinal;
	this->b_import_by_name = func.b_import_by_name;

    return *this;
}

void imported_func::set_hint(WORD hint) {
	this->hint = hint;
}
void imported_func::set_ordinal(WORD ordinal) {
	this->b_import_by_name = false;
	this->ordinal = ordinal;
}
void imported_func::get_name(std::string name) {
	this->b_import_by_name = true;
	this->name = name;
}
void imported_func::set_import_by_name(bool b) {
	this->b_import_by_name = b;
}
void imported_func::set_iat_rva(DWORD rva) {
	this->iat_rva = rva;
}
WORD imported_func::get_hint() const {
	return this->hint;
}
WORD imported_func::get_ordinal() const {
	return this->ordinal;
}
std::string imported_func::get_name() const {
	return this->name;
}
bool  imported_func::is_import_by_name() const {
	return this->b_import_by_name;
}
DWORD imported_func::get_iat_rva() const {
	return this->iat_rva;
}

imported_library::imported_library() {
	timestamp				            = 0;
    relative_virtual_address_to_iat		= 0;
	name.clear();
	imported_items.clear();
}
imported_library::~imported_library() {
	imported_items.clear();
}
imported_library& imported_library::operator=(const imported_library& library) {

	this->timestamp                       = library.timestamp;
	this->relative_virtual_address_to_iat = library.relative_virtual_address_to_iat;
	this->name                            = library.name;
	this->imported_items                  = library.imported_items;

    return *this;
}


void imported_library::set_name(std::string name){
	this->name = name;
}
void imported_library::set_timestamp(DWORD timestamp) {
	this->timestamp = timestamp;
}
void imported_library::set_rva_iat(DWORD relative_virtual_address) {
	this->relative_virtual_address_to_iat = relative_virtual_address;
}
void imported_library::add_item(const imported_func& item) {
	this->imported_items.push_back(item);
}

std::string imported_library::get_name() const {
	return this->name;
}
DWORD imported_library::get_timestamp() const {
	return this->timestamp;
}
DWORD imported_library::get_rva_iat() const {
	return this->relative_virtual_address_to_iat;
}
std::vector<imported_func>& imported_library::get_items() {
	return this->imported_items;
}


import_table::import_table() {

}
import_table::~import_table() {

}

import_table& import_table::operator=(const import_table& imports) {

	this->libs = imports.libs;

    return *this;
}

void import_table::add_lib(imported_library& lib) {
	this->libs.push_back(lib);
}

std::vector<imported_library>& import_table::get_libs() {
	return this->libs;
}

bool import_table::get_imported_lib(std::string lib_name, imported_library * &lib) {

	for (unsigned int i = 0; i < libs.size(); i++) {
		if (libs[i].get_name() == lib_name) {
			lib = &libs[i];
			return true;
		}
	}

	return false;
}
bool import_table::get_imported_func(std::string lib_name, std::string func_name, imported_library * &lib, imported_func * &func) {

	for (unsigned int i = 0; i < libs.size(); i++) {
		if (libs[i].get_name() == lib_name) {
			for (unsigned int j = 0; j < libs[i].get_items().size(); j++) {
				if (libs[i].get_items()[j].is_import_by_name() && libs[i].get_items()[j].get_name() == func_name) {
					lib = &libs[i];
					func = &libs[i].get_items()[j];
					return true;
				}
			}
		}
	}

	return false;
}
bool import_table::get_imported_func(std::string lib_name, WORD ordinal, imported_library * &lib, imported_func * &func) {

	for (unsigned int i = 0; i < libs.size(); i++) {
		if (libs[i].get_name() == lib_name) {
			for (unsigned int j = 0; j < libs[i].get_items().size(); j++) {
				if (!libs[i].get_items()[j].is_import_by_name() && libs[i].get_items()[j].get_ordinal() == ordinal) {
					lib = &libs[i];
					func = &libs[i].get_items()[j];
					return true;
				}
			}
		}
	}

	return false;
}


bool get_import_table(const pe_image &image, import_table& imports) {
	imports.get_libs().clear();
	
	DWORD virtual_address = image.get_directory_virtual_address(IMAGE_DIRECTORY_ENTRY_IMPORT);
	DWORD virtual_size = image.get_directory_virtual_size(IMAGE_DIRECTORY_ENTRY_IMPORT);

	if (virtual_address && virtual_size) {
		pe_section * imp_section = image.get_section_by_rva(virtual_address);
		if (imp_section) {
			BYTE * raw_decs = &imp_section->get_section_data().data()[virtual_address - imp_section->get_virtual_address()];

			for (unsigned int imp_size = 0; imp_size < virtual_size; imp_size += sizeof(IMAGE_IMPORT_DESCRIPTOR)) {
				PIMAGE_IMPORT_DESCRIPTOR imp_description = (PIMAGE_IMPORT_DESCRIPTOR)(&raw_decs[imp_size]);
				if (imp_description->FirstThunk && imp_description->Name) {
					pe_section * imp_name_section = image.get_section_by_rva(imp_description->Name);
					pe_section * imp_iat_section = image.get_section_by_rva(imp_description->FirstThunk);
                    pe_section * imp_oft_section = image.get_section_by_rva(imp_description->OriginalFirstThunk);

					imported_library lib;
					lib.set_name(std::string(((char*)&imp_name_section->get_section_data().data()[imp_description->Name - imp_name_section->get_virtual_address()])));
					lib.set_rva_iat(imp_description->FirstThunk);
                    lib.set_timestamp(imp_description->TimeDateStamp);

					void* lib_iat = &imp_name_section->get_section_data().data()[(imp_description->FirstThunk - imp_iat_section->get_virtual_address())];

                    if (imp_oft_section) {
                        lib_iat = &imp_oft_section->get_section_data().data()[(imp_description->OriginalFirstThunk - imp_oft_section->get_virtual_address())];
                    }

                    DWORD iat_ft_rva = imp_description->FirstThunk;
       
					if (image.is_x32_image()) {
                        for (; *(DWORD*)lib_iat; lib_iat = (void*)(&((DWORD*)lib_iat)[1]) ) {

							if (*(DWORD*)lib_iat&IMAGE_ORDINAL_FLAG32) {
								lib.add_item(imported_func(iat_ft_rva,0, *(DWORD*)lib_iat & 0xFFFF, 0));
							}
							else {
								pe_section * imp_func = image.get_section_by_rva(*(DWORD*)lib_iat);
								if (imp_func) {
									lib.add_item(imported_func(
                                        iat_ft_rva,
										(char*)&imp_func->get_section_data().data()[(*(DWORD*)lib_iat - imp_func->get_virtual_address()) + sizeof(WORD)],
										0,
										*(WORD*)&imp_func->get_section_data().data()[(*(DWORD*)lib_iat - imp_func->get_virtual_address())]
									));
								}
							}
                            iat_ft_rva += sizeof(DWORD);
						}
					}
					else {
                        for (; *(DWORD64*)lib_iat; lib_iat = (void*)(&((DWORD64*)lib_iat)[1])) {

							if (*(DWORD64*)lib_iat&IMAGE_ORDINAL_FLAG64) {
								lib.add_item(imported_func(iat_ft_rva, 0, *(DWORD64*)lib_iat & 0xFFFF, 0));
							}
							else {
								pe_section * imp_func = image.get_section_by_rva(*(DWORD*)lib_iat);
								if (imp_func) {
									lib.add_item(imported_func(
                                        iat_ft_rva,
										(char*)&imp_func->get_section_data().data()[(*(DWORD*)lib_iat - imp_func->get_virtual_address()) + sizeof(WORD)],
										0,
										*(WORD*)&imp_func->get_section_data().data()[(*(DWORD*)lib_iat - imp_func->get_virtual_address())]
									));
								}
							}

                            iat_ft_rva += sizeof(DWORD64);
						}
					}

					imports.add_lib(lib);
				}
				else {
					return true;
				}
			}
			return true;
		}
	}

	return false;
}
void build_import_table(pe_image &image, pe_section& section, import_table& imports) {

    if (!imports.get_libs().size()) { return; }

    if (section.get_size_of_raw_data() & 0xF) {
        section.get_section_data().resize(
            section.get_section_data().size() + (0x10 - (section.get_section_data().size() & 0xF))
        );
    }

	DWORD virtual_address = section.get_virtual_address() + section.get_size_of_raw_data();
	DWORD p_import_offset = section.get_size_of_raw_data();


    unsigned int desc_table_size = sizeof(IMAGE_IMPORT_DESCRIPTOR);
    unsigned int lib_names_size = 0;
    unsigned int func_names_size = 0;
    unsigned int thunk_size = 0;

    for (auto& lib : imports.get_libs()) {
        if (!lib.get_items().size()) { continue; }

        for (auto& func : lib.get_items()) {
            if (func.is_import_by_name()) {
                func_names_size += func.get_name().length() + 1 + sizeof(WORD);//hint
            }

            if (image.is_x32_image()) { thunk_size += sizeof(DWORD); }
            else { thunk_size += sizeof(DWORD64); }
        }
        
        if (image.is_x32_image()) { thunk_size += sizeof(DWORD); }
        else { thunk_size += sizeof(DWORD64); }

        lib_names_size += lib.get_name().length() + 1;
        desc_table_size += sizeof(IMAGE_IMPORT_DESCRIPTOR);
    }

    section.get_section_data().resize(section.get_size_of_raw_data() +
        desc_table_size +
        lib_names_size +
        func_names_size + 
        thunk_size*2);//oft and ft

    PIMAGE_IMPORT_DESCRIPTOR import_descs = (PIMAGE_IMPORT_DESCRIPTOR)&section.get_section_data().data()[p_import_offset];
    BYTE * lib_names = &section.get_section_data().data()[p_import_offset + desc_table_size];
    BYTE * func_names = &section.get_section_data().data()[p_import_offset + desc_table_size + lib_names_size];
    BYTE * iat_thunks = &section.get_section_data().data()[p_import_offset + desc_table_size + lib_names_size + func_names_size];

    for (auto& lib : imports.get_libs()) {
        if (!lib.get_items().size()) { continue; }

        import_descs->Characteristics = 0;
        import_descs->ForwarderChain = 0;
        import_descs->TimeDateStamp = lib.get_timestamp();
        import_descs->Name = section.get_virtual_address() + (lib_names - section.get_section_data().data());
        lstrcpy((char*)lib_names, lib.get_name().c_str());
        lib_names += lib.get_name().length() + 1;
        

        std::vector<DWORD64> iat_thunks_table;
        for (auto& func : lib.get_items()) {
            if (func.is_import_by_name()) {
                PIMAGE_IMPORT_BY_NAME pimport_by_name = (PIMAGE_IMPORT_BY_NAME)func_names;
                iat_thunks_table.push_back(section.get_virtual_address() + (func_names - section.get_section_data().data()));

                pimport_by_name->Hint = func.get_hint();
                lstrcpy(pimport_by_name->Name, func.get_name().c_str());

                func_names += sizeof(WORD) + func.get_name().length() + 1;
            }
            else {
                if (image.is_x32_image()) {
                    iat_thunks_table.push_back(func.get_ordinal() | IMAGE_ORDINAL_FLAG32);
                }
                else {
                    iat_thunks_table.push_back(func.get_ordinal() | IMAGE_ORDINAL_FLAG64);
                }
            }
        }
        iat_thunks_table.push_back(0);

        if (image.is_x32_image()) {
            import_descs->FirstThunk = section.get_virtual_address() + (iat_thunks - section.get_section_data().data());

            DWORD func_iat_rva = import_descs->FirstThunk;
            for (unsigned int func_idx = 0; func_idx < lib.get_items().size(); 
                func_idx++, iat_thunks += sizeof(DWORD), func_iat_rva += sizeof(DWORD)) {
                 
                *(DWORD64*)iat_thunks = (DWORD)iat_thunks_table[func_idx];

                 lib.get_items()[func_idx].set_iat_rva(func_iat_rva);
            }
            iat_thunks += sizeof(DWORD);


            import_descs->OriginalFirstThunk = section.get_virtual_address() + (iat_thunks - section.get_section_data().data());
            for (auto& item : iat_thunks_table) { *(DWORD*)iat_thunks = (DWORD)item; iat_thunks += sizeof(DWORD); }
        }
        else {
            import_descs->FirstThunk = section.get_virtual_address() + (iat_thunks - section.get_section_data().data());

            DWORD func_iat_rva = import_descs->FirstThunk;
            for (unsigned int func_idx = 0; func_idx < lib.get_items().size();
                func_idx++, iat_thunks += sizeof(DWORD64), func_iat_rva += sizeof(DWORD64)) {

                *(DWORD64*)iat_thunks = iat_thunks_table[func_idx];

                lib.get_items()[func_idx].set_iat_rva(func_iat_rva);
            }
            iat_thunks += sizeof(DWORD64);

            import_descs->OriginalFirstThunk = section.get_virtual_address() + (iat_thunks - section.get_section_data().data());
            for (auto& item : iat_thunks_table) { *(DWORD64*)iat_thunks = item; iat_thunks += sizeof(DWORD64); }
        }

        lib.set_rva_iat(import_descs->FirstThunk);
        import_descs++;
    }
  

    image.set_directory_virtual_address(IMAGE_DIRECTORY_ENTRY_IMPORT, virtual_address);
    image.set_directory_virtual_size(IMAGE_DIRECTORY_ENTRY_IMPORT, desc_table_size);
}

bool erase_import_table(pe_image &image, std::vector<erased_zone>* zones) {
	DWORD virtual_address = image.get_directory_virtual_address(IMAGE_DIRECTORY_ENTRY_IMPORT);
	DWORD virtual_size = image.get_directory_virtual_size(IMAGE_DIRECTORY_ENTRY_IMPORT);

	if (virtual_address && virtual_size) {
		

		pe_section * imp_section = image.get_section_by_rva(virtual_address);
		if (imp_section) {
			BYTE * raw_import = &imp_section->get_section_data().data()[virtual_address - imp_section->get_virtual_address()];
			for (unsigned int imp_size = 0; imp_size < virtual_size; imp_size += sizeof(IMAGE_IMPORT_DESCRIPTOR)) {
				PIMAGE_IMPORT_DESCRIPTOR imp_description = (PIMAGE_IMPORT_DESCRIPTOR)(&raw_import[imp_size]);
				if (imp_description->FirstThunk && imp_description->Name) {
					pe_section * imp_name_section = image.get_section_by_rva(imp_description->Name);
					pe_section * imp_iat_section  = image.get_section_by_rva(imp_description->FirstThunk);
					pe_section * imp_oft_section  = image.get_section_by_rva(imp_description->OriginalFirstThunk);


					if (zones) {
						zones->push_back({
							imp_description->Name ,
							(unsigned int)lstrlenA((char*)&imp_name_section->get_section_data().data()[(imp_description->Name - imp_name_section->get_virtual_address())]) + 1
						});
					}

					ZeroMemory((char*)&imp_name_section->get_section_data().data()[imp_description->Name - imp_name_section->get_virtual_address()],
						lstrlenA((char*)&imp_name_section->get_section_data().data()[(imp_description->Name - imp_name_section->get_virtual_address())]));



					void* lib_oft = 0;
					void* lib_iat = &imp_iat_section->get_section_data().data()[(imp_description->FirstThunk - imp_iat_section->get_virtual_address())];

					if (imp_oft_section) {
						lib_oft = &imp_oft_section->get_section_data().data()[(imp_description->OriginalFirstThunk - imp_oft_section->get_virtual_address())];
					}
					DWORD iat_ft_rva = imp_description->FirstThunk;
					DWORD iat_oft_rva = imp_description->OriginalFirstThunk;

					if (image.is_x32_image()) {
						while (*(DWORD*)lib_iat) {
							DWORD lib_iat_in = *(DWORD*)lib_iat;

							if ((DWORD)lib_iat_in&IMAGE_ORDINAL_FLAG32) {
								ZeroMemory(lib_iat, sizeof(DWORD));
							}
							else {
								pe_section * imp_func = image.get_section_by_rva((DWORD)lib_iat_in);
								if (imp_func) {

									if (zones) {
										zones->push_back({
											lib_iat_in ,
											(unsigned int)lstrlenA((char*)&imp_func->get_section_data().data()[((DWORD)lib_iat_in - imp_func->get_virtual_address()) + sizeof(WORD)]) + 2 + 2
										});
									}

									ZeroMemory((char*)&imp_func->get_section_data().data()[((DWORD)lib_iat_in - imp_func->get_virtual_address())],
										lstrlenA((char*)&imp_func->get_section_data().data()[((DWORD)lib_iat_in - imp_func->get_virtual_address()) + sizeof(WORD)]) + 2);
								}
							}



							if (lib_oft) {
								if (zones) {
									zones->push_back({
										iat_oft_rva ,
										sizeof(DWORD)
									});
								}

								((DWORD*)lib_oft)[0] = 0;
								lib_oft = (void*)(&((DWORD*)lib_oft)[1]);
								iat_oft_rva += sizeof(DWORD);
							}


							if (zones) {
								zones->push_back({
									iat_ft_rva ,
									sizeof(DWORD)
								});
							}
							((DWORD*)lib_iat)[0] = 0;
							lib_iat = (void*)(&((DWORD*)lib_iat)[1]);
							iat_ft_rva += sizeof(DWORD);
						}
						if (zones) {
							zones->push_back({
								iat_ft_rva ,
								sizeof(DWORD)
							});
						}
						if (lib_oft && zones) {
							zones->push_back({
								iat_oft_rva ,
								sizeof(DWORD)
							});
						}
					}
					else {
						while (*(DWORD64*)lib_iat) {
							DWORD64 lib_iat_in = *(DWORD64*)lib_iat;

							if ((DWORD64)lib_iat_in&IMAGE_ORDINAL_FLAG64) {
								ZeroMemory(lib_iat, sizeof(DWORD64));
							}
							else {
								pe_section * imp_func = image.get_section_by_rva((DWORD)lib_iat_in);
								if (imp_func) {

									if (zones) {
										zones->push_back({
											(DWORD)lib_iat_in ,
											(unsigned int)lstrlenA((char*)&imp_func->get_section_data().data()[((DWORD)lib_iat_in - imp_func->get_virtual_address()) + sizeof(WORD)]) + 2 + 2
										});
									}

									ZeroMemory((char*)&imp_func->get_section_data().data()[((DWORD)lib_iat_in - imp_func->get_virtual_address())],
										lstrlenA((char*)&imp_func->get_section_data().data()[((DWORD)lib_iat_in - imp_func->get_virtual_address()) + sizeof(WORD)]) + 2);
								}
							}


							
							if (lib_oft) {
								if (zones) {
									zones->push_back({
										iat_oft_rva ,
										sizeof(DWORD64)
									});
								}

								((DWORD64*)lib_oft)[0] = 0;
								lib_oft = (void*)(&((DWORD64*)lib_oft)[1]);
								iat_oft_rva += sizeof(DWORD64);
							}

							if (zones) {
								zones->push_back({
									iat_ft_rva ,
									sizeof(DWORD64)
								});
							}

							((DWORD64*)lib_iat)[0] = 0;
							lib_iat = (void*)(&((DWORD64*)lib_iat)[1]);
							iat_ft_rva += sizeof(DWORD64);
						}

						if (zones) {
							zones->push_back({
								iat_ft_rva ,
								sizeof(DWORD64)
							});
						}
						if (lib_oft && zones) {
							zones->push_back({
								iat_oft_rva ,
								sizeof(DWORD64)
							});
						}
					}

					ZeroMemory(imp_description, sizeof(IMAGE_IMPORT_DESCRIPTOR));
				}
				else {
					if (zones) { zones->push_back({ virtual_address ,virtual_size }); }
					image.set_directory_virtual_address(IMAGE_DIRECTORY_ENTRY_IMPORT, 0);
					image.set_directory_virtual_size(IMAGE_DIRECTORY_ENTRY_IMPORT, 0);
                    image.set_directory_virtual_address(IMAGE_DIRECTORY_ENTRY_IAT, 0);
                    image.set_directory_virtual_size(IMAGE_DIRECTORY_ENTRY_IAT, 0);
					return true;
				}
			}
			if (zones) { zones->push_back({ virtual_address ,virtual_size }); }
			image.set_directory_virtual_address(IMAGE_DIRECTORY_ENTRY_IMPORT, 0);
			image.set_directory_virtual_size(IMAGE_DIRECTORY_ENTRY_IMPORT, 0);
            image.set_directory_virtual_address(IMAGE_DIRECTORY_ENTRY_IAT, 0);
            image.set_directory_virtual_size(IMAGE_DIRECTORY_ENTRY_IAT, 0);
			return true;
		}
	}
	return false;
}

