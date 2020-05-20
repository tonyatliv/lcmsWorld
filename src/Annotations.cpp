#include "Annotations.h"



#include <vector>
#include <cstdio>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include "SampleLoader.h"
#include "Error.h"

#include "Structs.h"
#include "SystemSetup.h"
#include "Zip.h"

#include "rapidxml.hpp"

std::map<std::string, int> sequenceTypes;
using namespace rapidxml;
int numPeptides = 0;
int numPE = 0;
int numDB = 0;
int numS = 0;


std::map<std::string, std::string> peptideSequences;
std::map<std::string, std::string> peptideMods;
std::map<std::string, std::string> peptideEvidence;
std::map<std::string, std::string> dbSequence;


std::map<std::string, std::string> peptide_RT;
std::map<std::string, std::string> peptide_mz;
std::map<std::string, std::string> peptide_score;
std::map<std::string, std::string> peptide_peptideEvidence;

void parsePeptide(xml_node<>* node)
{
	auto ida = node->first_attribute("id");
	if (ida == NULL)
	{
		std::cout << " node without id in .mzid file, ignoring \n";
		return;

	}
	std::string id = ida->value();



	auto seq = node->first_node("PeptideSequence");
	if (seq == NULL)
	{
		std::cout << " node without PeptideSequence in .mzid file, ignoring \n";
		return;
	}

	std::string sequence = seq->value();

	auto mod = node->first_node("Modification");
	std::string modText = "";
	while (mod != NULL)
	{
		auto cv = mod->first_node("cvParam");
		if (cv == NULL)
			continue;
		auto moda = cv->first_attribute("name");
		if (moda == NULL)
			continue;
		std::string modname = moda->value();
		ltrim(modname);
		rtrim(modname);
		if (modText.length() > 0)
			modText += " ";
		modText += modname;
		mod = mod->next_sibling();
	}

	peptideSequences[id] = sequence;
	peptideMods[id] = modText;
	// std::cout << modText << " modTest \n";
	numPeptides++;

}
void parsePeptideEvidence(xml_node<>* node)
{
	//	<PeptideEvidence id = "gi|392920101|gn|mec-9:179382|_PEP_1" peptide_ref = "PEP_1" dBSequence_ref = "DBSeq_gi|392920101|gn|mec-9:179382|" pre = "K" post = "E" isDecoy = "false" / >
	auto idn = node->first_attribute("peptide_ref");
	auto dBSequence_refn = node->first_attribute("dBSequence_ref");
	if ((idn == NULL) || (dBSequence_refn == NULL) )
	{
	std::cout << " PeptideEvidence without peptide_ref,dBSequence_ref in .mzid file, ignoring \n";
	return;
	}
	auto id = idn->value();
	auto dBSequence_ref = dBSequence_refn->value();
	peptideEvidence[id] = dBSequence_ref;
	numPE++;

}
void parseDBSequence(xml_node<>* node)
{
	//    <DBSequence id="DBSeq_gi|392920101|gn|mec-9:179382|" accession="gi|392920101|gn|mec-9:179382|" searchDatabase_ref="DB_1">

	/*
	<DBSequence id="DBSeq_gi|392920101|gn|mec-9:179382|" accession="gi|392920101|gn|mec-9:179382|" searchDatabase_ref="DB_1">
      <cvParam cvRef="MS" accession="MS:1001088" name="protein description" value="Uncharacterized protein CELE_C50H2.3 [Caenorhabditis elegans]"/>
    </DBSequence>
	*/
	auto idn = node->first_attribute("id");
	
	auto accession_node = node->first_attribute("accession");
	if ((idn == NULL) || (accession_node == NULL))
	{
		std::cout << " DBSequence without id,accession in .mzid file, ignoring \n";
		return;
	}
	auto id = idn->value();
	auto accession = accession_node->value();
	dbSequence[id] = accession;
 

	numDB++;

}
 


void parseSpectrum(xml_node<>* node)
{
	numS++;

	std::string RT = "";

	//check cvparams for RT
	auto cvNode = node->first_node("cvParam");
	while (cvNode != NULL)
	{
		if (cvNode->first_attribute("accession") != NULL)
		{
		std::string type = cvNode->first_attribute("accession")->value();
		if (type == "MS:1000016" || type == "MS:1000894" || type == "MS:1000895" || type == "MS:1000826" || type == "MS:1000916" || type == "MS:1001114")
			{

				if (cvNode->first_attribute("value") != NULL)
				{


					auto time = cvNode->first_attribute("value")->value();
					auto scale = cvNode->first_attribute("unitName");
					double rt = std::stod(time);
					if (scale != NULL)
						if (scale->value() == "minute")
							rt = rt / 60;
					RT = std::to_string(rt);
				}
			}
		}

		cvNode = cvNode->next_sibling();
	}

	if (RT == "")
	{
		std::cout << " SpectrumIdentificationResult with no retention time, ignoring \n";
		return;
	}

	node = node->first_node("SpectrumIdentificationItem");
	if (node == NULL)
	{
		std::cout << " SpectrumIdentificationResult without  SpectrumIdentificationItem n in .mzid file, ignoring \n";

		return;
	}


	//
	/*
if (sequenceTypes.find(name) == sequenceTypes.end())
	sequenceTypes.insert(std::pair<std::string, int>(name, 0));


sequenceTypes[name] = sequenceTypes[name] + 1;

*/
	std::string score = "0";
	auto cv = node->first_node("cvParam");
	while (cv != NULL)
	{
		auto cva = cv->first_attribute("accession");
		if (cva != NULL)
		{

 


			if (cva->value() == "MS:1002255" 
				|| cva->value() == "MS:1002356"
				|| cva->value() == "MS:1002360"
				|| cva->value() == "MS:1002049"
				|| cva->value() == "MS:1002545"
				)
			{
				cva = cv->first_attribute("value");
				if (cva != NULL)
					score = cva->value();
			}
		}

		cv = cv->next_sibling();
	}



	auto mz = node->first_attribute("experimentalMassToCharge");
	if (mz == NULL)		return;
	auto mzVal = mz->value();

	auto refa = node->first_attribute("peptide_ref");
	if (refa == NULL)		return;
	auto id = refa->value();

	

	auto pe = node->first_node("PeptideEvidenceRef");
	if (pe == NULL)
	{
		return;
	}
	auto pea = pe->first_attribute("peptideEvidence_ref");
	if (pea == NULL) return;
	auto pepRef = pea->value();
	
	peptide_RT[id] = RT;
	peptide_mz[id] = mzVal;
	peptide_peptideEvidence[id] = pepRef;

	peptide_score[id] = score;



}
 
void parseNextLevel(xml_node<>* node)
{

	while (node != 0) {

		xml_node<>* child = node->first_node();


		std::string name = child->name();


		while (child != 0)
		{
			std::string name = child->name();
			if (name == "Peptide")
			{
				parsePeptide(child);
			}
			if (name == "PeptideEvidence")
			{
				parsePeptideEvidence(child);
			}
			if (name == "DBSequence")
			{
				parseDBSequence(child);
			}
			if (name == "SpectrumIdentificationResult")
			{
				parseSpectrum(child);
			}
			
	

			child = child->next_sibling();
		}
		node = node->next_sibling();
	}

}

void printLevel(xml_node<>* node)
{
	std::cout << " children of " << node->name() << "\n";
	xml_node<>* child = node->first_node();

	while (child != 0)
	{
		std::string name = child->name();
		std::cout << name << " \n";
		child = child->next_sibling();
	}


	std::cout <<  " \n";
}

void Annotations::loadMZIDFromBuffer(char* buffer, Landscape* l)
{
//	try
	{
		xml_document<> doc;    // character type defaults to char

		doc.parse<0>(buffer);    // 0 means default parse flags

		xml_node<>* root = doc.first_node("MzIdentML");


		xml_node<>* node = root->first_node("SequenceCollection");
		parseNextLevel(node);

		node = root->first_node("DataCollection");
		node = node->first_node("AnalysisData");
		node = node->first_node("SpectrumIdentificationList");



		parseNextLevel(node);


		if (peptide_RT.size() == 0)
		{
			new Error(Error::ErrorType::file, "This file does not have the required information\nto support annotations with lcmsWorld.\
\npeptide, m/z, and RT are required.");
			return;
		}

		for (auto pair : peptide_RT)
		{

			auto id = pair.first;
			mzFloat mz = std::stod(peptide_mz[id]);
			lcFloat lc = (lcFloat)std::stod(pair.second);

			signalFloat intensity = 0;

			auto seq = peptideSequences[id];
			auto ptm = peptideMods[id];

			auto accession = peptide_peptideEvidence[id];


			auto evv = dbSequence[accession];


			std::string name = seq;
			std::string text = name;
			if (text.length() > 50)
			{
				int point = text.length() / 2;
				while (std::isalpha(text[point]))
				{
					point++;
				}
				name = text.substr(0, point);
				text = text.substr(0, point) + "\n" + text.substr(point);
			}
			double score = std::stod(peptide_score[id]);

			ImVec2 size = ImGui::CalcTextSize(name.c_str());
			Annotation a = { mz, lc, intensity, score, text,ptm,accession,"", size.x,size.y, 0 };
			l->addAnnotation(a);

		}



		l->setVisible(0);

		l->setAnnotationsLoaded(1.0 / 60);
	} 
//	catch (...)
//	{
//		new Error(Error::ErrorType::file, "There was a problem parsing this file.");
//	}
}

void Annotations::loadMZID_bg(std::string filename, Landscape* l)
{
 
//	try
	{
		std::fstream f;

		filename = convertFilename(filename);

	


		long long size = 0;
		long long readBytes = 0;

		std::ifstream* filePtr;

		filePtr = new std::ifstream(filename, std::ios::binary | std::ios::ate);
		size = filePtr->tellg();
		char* buffer = (char*)malloc(size + 1);
		readBytes = 0;
		filePtr->seekg(0, std::ios::beg);

		if (filePtr->read(buffer, size))
		{
			//read ok
		}
		filePtr->close();
		buffer[size] = 0;
		std::cout << size << " read bytes for mzid " << filename << "\n";

 

		if (endsWith(filename, ".gz"))
		{
			unsigned int* sz_ptr = (unsigned int*) &buffer[size - 4];
			int decodeSize = *sz_ptr;
			

			std::cout << " Uncompress from " << size << " to " << decodeSize << "\n";

			char* zipBuffer = (char*) malloc(decodeSize+1);
		 
				int length = Zip::UncompressGzipData((BYTE*) buffer, size, (BYTE*) &zipBuffer[0], decodeSize);
				 
		 
				free(buffer);
				buffer = zipBuffer;
				buffer[decodeSize] = 0;
		}
		loadMZIDFromBuffer(buffer, l);
		free(buffer);

	}
//	catch (...)
//	{
//		new Error(Error::ErrorType::file, "There was a problem loading this file.");
//	}


}

 
void Annotations::loadMZID(std::string filename, Landscape* l)
{
	std::thread t1(Annotations::loadMZID_bg, filename, l);

	t1.detach();

}

void Annotations::loadTextBased(std::string filename, char delim, Landscape* l)
{
	std::fstream f;

	filename = convertFilename(filename);
	f.open(filename, std::ios::in);

	try
	{
		std::string   line;
		std::vector<std::string> header;

		std::getline(f, line);
		std::map<std::string, int> hd;


		header = Utils::split(line, delim);
		int pep_col = -1;
		int score_col = -1;
		int mz_col = -1;
		int lc_col = -1;
		int intensity_col = -1;

		int ptm_col = -1;
		int accession_col = -1;
		for (unsigned int i = 0; i < header.size(); i++)
		{
			rtrim(header[i]);
			header[i] = tolower(header[i]);



			if ((header[i] == "peptide")
				|| (header[i] == "sequence")
				|| (header[i] == "modified sequence")
				)
				pep_col = i;


			if ((header[i] == "-10lgp")
				|| (header[i] == "score")
				|| (header[i] == "local confidence(%)")
				)

				score_col = i;

			if (header[i] == "m/z")
				mz_col = i;
			if ((header[i] == "rt")
				|| (header[i] == "retention time"))
				lc_col = i;
			if (header[i] == "intensity")
				intensity_col = i;

			if ((header[i] == "accession")
				|| (header[i] == "protein accession"))
				accession_col = i;
			if (header[i] == "ptm")
				ptm_col = i;

		}


		int numAnnotations = 0;
		if ((mz_col < 0) || (lc_col < 0) || (pep_col < 0))
		{
			new Error(Error::ErrorType::file, "This file does not have the required information\nto support annotations with lcmsWorld.\
\npeptide, m/z, and RT are required.");
			try
			{
				f.close();
			}
			catch (...) {}

			return;
		}

		mzFloat maxLc = 0;

		while (std::getline(f, line))
		{
			std::vector<std::string> cells;
			cells = Utils::split(line, delim);

			if (cells.size() < 3)
				continue;
			try
			{
				auto mz_s = cells[mz_col];
				auto lc_s = cells[lc_col];
				std::string name = cells[pep_col];
				std::string ptm;
				if (ptm_col > -1)
					ptm = cells[ptm_col];

				std::string accession;
				if (accession_col > -1)
					accession = cells[accession_col];

				if (name.length() > 1300)
				{
					std::cout << " too long " << name << "\n";
					continue;
				}

				std::string score_s = "0";
				if (score_col > -1)
					score_s = cells[score_col];

				float score = std::stof(score_s.c_str());

				mzFloat mz = std::stof(mz_s.c_str());
				lcFloat lc = std::stof(lc_s.c_str());

				signalFloat intensity = 0;
				if (intensity_col > -1)
					intensity = std::stof(cells[intensity_col].c_str());;

				std::string text = name;
				if (text.length() > 50)
				{
					int point = text.length() / 2;
					while (std::isalpha(text[point]))
					{
						point++;
					}
					name = text.substr(0, point);
					text = text.substr(0, point) + "\n" + text.substr(point);
				}


				maxLc = std::max(maxLc, lc);

				ImVec2 size = ImGui::CalcTextSize(name.c_str());


				Annotation a = { mz, lc, intensity, score, text,ptm,accession,"", size.x,size.y, 0 };
				l->addAnnotation(a);
				numAnnotations++;

			}
			catch (...) {
				// probably null ?
				new Error(Error::ErrorType::file, "There was an error reading the annotations from the file.");
				try
				{
					f.close();
				}
				catch (...) {}

				return;
			}

		}
		l->setVisible(0);

 
		lcFloat scale = 1.0;
		if (maxLc > System::primary->worldLcRange.max * 1.3)  // must be in seconds, convert to minutes
			scale = 1.0 / 60;
		std::cout << " Max RT = " << maxLc << " scaling RT by " << scale << "\n";

		l->setAnnotationsLoaded(scale);

		f.close();

	}
	catch (...) {
		// no idea, but best to report something

		new Error(Error::ErrorType::file, "There was an error reading the annotations from the file.");
		try
		{
			f.close();
		}
		catch (...) {}
	}
		
}




void Annotations::loadCSV(std::string filename, Landscape* l)
{
	loadTextBased(filename, ',', l);
}









void Annotations::loadText(std::string filename, Landscape* l)
{
	loadTextBased(filename, '\t', l);

}


void  Annotations::loadCSVA(std::string filename, Landscape* l)
{

	std::thread t1(Annotations::loadCSV, filename, l);

	t1.detach();

}

void  Annotations::loadTextA(std::string filename, Landscape* l)
{

	std::thread t1(Annotations::loadText, filename, l);

	t1.detach();

}

void Annotations::loadMZTab(std::string filename, Landscape* l)
{
	std::fstream f;

	filename = convertFilename(filename);

	f.open(filename, std::ios::in);
	std::string   line;
	std::vector<std::string> header;
	while (std::getline(f, line))
	{
		std::istringstream ss(line);
		std::string substr;
		if (startsWith(line, "PEH\t"))
		{
			while (std::getline(ss, substr, '\t'))
			{
				header.push_back(substr);

			}
			break;
		}
	}
	if (header.size() == 0)
	{
		reportError("Invalid mztab file");
		std::cout << " no PEH header found \n";
		new Error(Error::ErrorType::file, "Invalid mztab file");
		return;
	}

	int name_col = -1;
	int mz_col = -1;
	int lc_col = -1;

	for (unsigned int i = 0; i < header.size(); i++)
	{
		if (header[i] == "sequence")
			name_col = i;
		if (header[i] == "mass_to_charge")
			mz_col = i;
		if (header[i] == "retention_time")
			lc_col = i;
	}

	if (name_col < 0)
	{
		reportError("Invalid mztab file");
		std::cout << " sequence not found in mztab\n"; return;
	}
	if (mz_col < 0)
	{
		reportError("Invalid mztab file");
		std::cout << " mz_col not found in mztab\n"; return;
	}
	if (lc_col < 0)
	{
		reportError("Invalid mztab file");
		std::cout << " lc_col not found in mztab\n"; return;
	}


	if ((name_col < 0)
		|| (mz_col < 0)
		|| (lc_col < 0))
	{
		new Error(Error::ErrorType::file, "This mztab file does not have the required information\nto support annotations with lcmsWorld.");

		return;

	}
	while (std::getline(f, line))
	{
		std::istringstream ss(line);
		std::string substr;
		int i = 0;

		std::string name;
		std::string mz_s;
		std::string lc_s;

		while (getline(ss, substr, '\t'))
		{
			if (i == name_col)
				name = substr;
			if (i == mz_col)
				mz_s = substr;
			if (i == lc_col)
				lc_s = substr;
			i++;
		}
		if ((mz_s.length() == 0) || (name.length() == 0) || (lc_s.length() == 0))
			continue;

		try
		{
			mzFloat mz = std::stof(mz_s.c_str());
			lcFloat lc = std::stof(lc_s.c_str());
			lc = lc / 60;
			ImVec2 size = ImGui::CalcTextSize(name.c_str());

			//todo - add score etc
			Annotation a = { mz, lc, (signalFloat)0.0, 0, name,"","","",size.x,size.y, 0 };
			l->addAnnotation(a);
		}
		catch (...) {
			// probably null ?
		}


	}


	f.close();


}

