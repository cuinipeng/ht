/*
 *	HT_Editor
 *	htpe.cc
 *
 *	Copyright (C) 2003 Stefan Esser
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2 as
 *	published by the Free Software Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "log.h"
#include "htendian.h"
#include "htxbe.h"
#include "htxbehead.h"
#include "stream.h"
#include "tools.h"

#include <stdlib.h>
#include <string.h>

format_viewer_if *htxbe_ifs[] = {
	&htxbeheader_if,
//	&htpeexports_if,
//	&htpeimports_if,
//	&htpedelayimports_if,
//	&htperesources_if,
//	&htpeil_if,
//	&htpeimage_if,
	0
};

ht_view *htxbe_init(bounds *b, ht_streamfile *file, ht_format_group *format_group)
{
	byte xbemagic[4];

	file->seek(0);
	file->read(xbemagic, 4);
	if ((xbemagic[0]!=XBE_MAGIC0) || (xbemagic[1]!=XBE_MAGIC1) ||
	   (xbemagic[2]!=XBE_MAGIC2) || (xbemagic[3]!=XBE_MAGIC3)) return 0;

	ht_xbe *g=new ht_xbe();
	g->init(b, file, htxbe_ifs, format_group, 0);
	return g;
}

format_viewer_if htxbe_if = {
	htxbe_init,
	0
};

/*
 *	CLASS ht_xbe
 */
void ht_xbe::init(bounds *b, ht_streamfile *file, format_viewer_if **ifs, ht_format_group *format_group, FILEOFS header_ofs)
{
	ht_format_group::init(b, VO_BROWSABLE | VO_SELECTABLE | VO_RESIZE, DESC_XBE, file, false, true, 0, format_group);
	VIEW_DEBUG_NAME("ht_xbe");

	ht_xbe_shared_data *xbe_shared = (ht_xbe_shared_data*)malloc(sizeof (ht_xbe_shared_data));
	shared_data = xbe_shared;

/*
	xbe_shared->exports.funcs = new ht_clist();
	xbe_shared->exports.funcs->init();

	xbe_shared->dimports.funcs = new ht_clist();
	xbe_shared->dimports.funcs->init();

	xbe_shared->dimports.libs = new ht_clist();
	xbe_shared->dimports.libs->init();

	xbe_shared->imports.funcs = new ht_clist();
	xbe_shared->imports.funcs->init();

	xbe_shared->imports.libs = new ht_clist();
	xbe_shared->imports.libs->init();

	xbe_shared->il = NULL;
	xbe_shared->v_image = NULL;
	xbe_shared->v_dimports = NULL;
	xbe_shared->v_imports = NULL;
	xbe_shared->v_exports = NULL;
	xbe_shared->v_resources = NULL;
	xbe_shared->v_header = NULL;
*/

/* read header */
	file->seek(0);
	file->read(&xbe_shared->header, sizeof xbe_shared->header);
	create_host_struct(&xbe_shared->header.base_address, XBE_IMAGE_HEADER_struct, little_endian);

/* read headerspace XXX: UGLY*/
	file->seek(0);
	xbe_shared->headerspace=(char *)malloc(xbe_shared->header.size_of_headers+1);
	file->read(xbe_shared->headerspace, xbe_shared->header.size_of_headers);
	xbe_shared->headerspace[xbe_shared->header.size_of_headers]=0;

/* read certificate */	
	file->seek(xbe_shared->header.certificate_address-xbe_shared->header.base_address);
	file->read(&xbe_shared->certificate, sizeof xbe_shared->certificate);
	create_host_struct(&xbe_shared->certificate, XBE_CERTIFICATE_struct, little_endian);

/* read library versions */
	file->seek(xbe_shared->header.library_versions_address-xbe_shared->header.base_address);
	
	xbe_shared->libraries=(XBE_LIBRARY_VERSION*)malloc(xbe_shared->header.number_of_library_versions * sizeof *xbe_shared->libraries);
	file->read(xbe_shared->libraries, xbe_shared->header.number_of_library_versions * sizeof *xbe_shared->libraries);

	for (UINT i=0; i<xbe_shared->header.number_of_library_versions; i++) {
		create_host_struct(&xbe_shared->libraries[i], XBE_LIBRARY_VERSION_struct, little_endian);
	}

/* read section headers */
	file->seek(xbe_shared->header.section_header_address-xbe_shared->header.base_address);
	
	xbe_shared->sections.sections=(XBE_SECTION_HEADER*)malloc(xbe_shared->header.number_of_sections * sizeof *xbe_shared->sections.sections);
	file->read(xbe_shared->sections.sections, xbe_shared->header.number_of_sections * sizeof *xbe_shared->sections.sections);

	xbe_shared->sections.number_of_sections=xbe_shared->header.number_of_sections;
	xbe_shared->sections.base_address=xbe_shared->header.base_address;

	for (UINT i=0; i<xbe_shared->header.number_of_sections; i++) {
		create_host_struct(&xbe_shared->sections.sections[i], XBE_SECTION_HEADER_struct, little_endian);
	}

	shared_data = xbe_shared;

	ht_format_group::init_ifs(ifs);
}

void ht_xbe::done()
{
	ht_format_group::done();

	ht_xbe_shared_data *xbe_shared = (ht_xbe_shared_data*)shared_data;

/*
	if (pe_shared->exports.funcs) {
		pe_shared->exports.funcs->destroy();
		delete pe_shared->exports.funcs;
	}
	if (pe_shared->dimports.funcs) {
		pe_shared->dimports.funcs->destroy();
		delete pe_shared->dimports.funcs;
	}
	if (pe_shared->dimports.libs) {
		pe_shared->dimports.libs->destroy();
		delete pe_shared->dimports.libs;
	}
	if (pe_shared->imports.funcs) {
		pe_shared->imports.funcs->destroy();
		delete pe_shared->imports.funcs;
	}
	if (pe_shared->imports.libs) {
		pe_shared->imports.libs->destroy();
		delete pe_shared->imports.libs;
	}
*/
	free(xbe_shared->sections.sections);

	free(shared_data);
}

void ht_xbe::loc_enum_start()
{
/*
	ht_pe_shared_data *sh=(ht_pe_shared_data*)shared_data;
	if (sh->opt_magic==COFF_OPTMAGIC_PE32) {
		loc_enum=1;
	} else {
		loc_enum=0;
	}
*/
}

bool ht_xbe::loc_enum_next(ht_format_loc *loc)
{
	return false;
}

/*
 *	rva conversion routines
 */

bool xbe_rva_to_ofs(xbe_section_headers *section_headers, RVA rva, FILEOFS *ofs)
{
	XBE_SECTION_HEADER *s=section_headers->sections;
	for (UINT i=0; i<section_headers->number_of_sections; i++) {
		if ((rva>=s->virtual_address) &&
		(rva<s->virtual_address+s->raw_size)) {
			*ofs=rva-s->virtual_address+s->raw_address;
			return true;
		}
		s++;
	}
	return false;
}

bool xbe_rva_to_section(xbe_section_headers *section_headers, RVA rva, int *section)
{
	XBE_SECTION_HEADER *s=section_headers->sections;
	for (UINT i=0; i<section_headers->number_of_sections; i++) {
		if ((rva>=s->virtual_address) &&
		(rva<s->virtual_address+MAX(s->virtual_size, s->raw_size))) {
			*section=i;
			return true;
		}
		s++;
	}
	return false;
}

bool xbe_rva_is_valid(xbe_section_headers *section_headers, RVA rva)
{
	XBE_SECTION_HEADER *s=section_headers->sections;
	for (UINT i=0; i<section_headers->number_of_sections; i++) {
		if ((rva>=s->virtual_address) &&
		(rva<s->virtual_address+MAX(s->virtual_size, s->raw_size))) {
			return true;
		}
		s++;
	}
	return false;
}

bool xbe_rva_is_physical(xbe_section_headers *section_headers, RVA rva)
{
	XBE_SECTION_HEADER *s=section_headers->sections;
	for (UINT i=0; i<section_headers->number_of_sections; i++) {
		if ((rva>=s->virtual_address) &&
		(rva<s->virtual_address+s->raw_size)) {
			return true;
		}
		s++;
	}
	return false;
}

/*
 *	ofs conversion routines
 */

bool xbe_ofs_to_rva(xbe_section_headers *section_headers, FILEOFS ofs, RVA *rva)
{
	XBE_SECTION_HEADER *s=section_headers->sections;
	for (UINT i=0; i<section_headers->number_of_sections; i++) {
		if ((ofs>=s->raw_address) &&
		(ofs<s->raw_address+s->raw_size)) {
			*rva=ofs-s->raw_address+s->virtual_address;
			return true;
		}
		s++;
	}
	return false;
}

bool xbe_ofs_to_section(xbe_section_headers *section_headers, FILEOFS ofs, int *section)
{
	XBE_SECTION_HEADER *s=section_headers->sections;
	for (UINT i=0; i<section_headers->number_of_sections; i++) {
		if ((ofs>=s->raw_address) &&
		(ofs<s->raw_address+s->raw_size)) {
			*section=i;
			return true;
		}
		s++;
	}
	return false;
}

bool xbe_ofs_to_rva_and_section(xbe_section_headers *section_headers, FILEOFS ofs, RVA *rva, int *section)
{
	bool r = xbe_ofs_to_rva(section_headers, ofs, rva);
	if (r) {
		r = xbe_ofs_to_section(section_headers, ofs, section);
	}
	return r;
}

bool xbe_ofs_is_valid(xbe_section_headers *section_headers, FILEOFS ofs)
{
	RVA rva;
	return xbe_ofs_to_rva(section_headers, ofs, &rva);
}

/*
 *
 */
 
bool xbe_section_name_to_section(xbe_section_headers *section_headers, const char *name, int *section)
{
	XBE_SECTION_HEADER *s = section_headers->sections;
	int slen = strlen(name);

	for (UINT i=0; i < section_headers->number_of_sections; i++) {
		if (strncmp(name, (char*)&s->section_name_address-section_headers->base_address, slen) == 0) {
			*section = i;
			return true;
		}
		s++;
	}
	return false;
}
