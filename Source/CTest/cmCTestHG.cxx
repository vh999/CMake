/*=========================================================================

  Program:   CMake - Cross-Platform Makefile Generator
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Kitware, Inc. All rights reserved.
  See Copyright.txt or http://www.cmake.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "cmCTestHG.h"

#include "cmCTest.h"
#include "cmSystemTools.h"
#include "cmXMLParser.h"

#include <cmsys/RegularExpression.hxx>

//----------------------------------------------------------------------------
cmCTestHG::cmCTestHG(cmCTest* ct, std::ostream& log):
  cmCTestGlobalVC(ct, log)
{
  this->PriorRev = this->Unknown;
}

//----------------------------------------------------------------------------
cmCTestHG::~cmCTestHG()
{
}

//----------------------------------------------------------------------------
class cmCTestHG::IdentifyParser: public cmCTestVC::LineParser
{
public:
  IdentifyParser(cmCTestHG* hg, const char* prefix,
                 std::string& rev): Rev(rev)
    {
    this->SetLog(&hg->Log, prefix);
    this->RegexIdentify.compile("^([0-9a-f]+)");
    }
private:
  std::string& Rev;
  cmsys::RegularExpression RegexIdentify;

  bool ProcessLine()
    {
    if(this->RegexIdentify.find(this->Line))
      {
      this->Rev = this->RegexIdentify.match(1);
      return false;
      }
    return true;
    }
};

//----------------------------------------------------------------------------
class cmCTestHG::StatusParser: public cmCTestVC::LineParser
{
public:
  StatusParser(cmCTestHG* hg, const char* prefix): HG(hg)
    {
    this->SetLog(&hg->Log, prefix);
    this->RegexStatus.compile("([MARC!?I]) (.*)");
    }

private:
  cmCTestHG* HG;
  cmsys::RegularExpression RegexStatus;

  bool ProcessLine()
    {
    if(this->RegexStatus.find(this->Line))
      {
      this->DoPath(this->RegexStatus.match(1)[0],
                   this->RegexStatus.match(2));
      }
    return true;
    }

  void DoPath(char status, std::string const& path)
    {
    if(path.empty()) return;

    // See "hg help status".  Note that there is no 'conflict' status.
    switch(status)
      {
      case 'M': case 'A': case '!': case 'R':
        this->HG->DoModification(PathModified, path);
        break;
      case 'I': case '?': case 'C': case ' ': default:
        break;
      }
    }
};

//----------------------------------------------------------------------------
std::string cmCTestHG::GetWorkingRevision()
{
  // Run plumbing "hg identify" to get work tree revision.
  const char* hg = this->CommandLineTool.c_str();
  const char* hg_identify[] = {hg, "identify","-i", 0};
  std::string rev;
  IdentifyParser out(this, "rev-out> ", rev);
  OutputLogger err(this->Log, "rev-err> ");
  this->RunChild(hg_identify, &out, &err);
  return rev;
}

//----------------------------------------------------------------------------
void cmCTestHG::NoteOldRevision()
{
  this->OldRevision = this->GetWorkingRevision();
  cmCTestLog(this->CTest, HANDLER_OUTPUT, "   Old revision of repository is: "
             << this->OldRevision << "\n");
  this->PriorRev.Rev = this->OldRevision;
}

//----------------------------------------------------------------------------
void cmCTestHG::NoteNewRevision()
{
  this->NewRevision = this->GetWorkingRevision();
  cmCTestLog(this->CTest, HANDLER_OUTPUT, "   New revision of repository is: "
             << this->NewRevision << "\n");
}

//----------------------------------------------------------------------------
bool cmCTestHG::UpdateImpl()
{
  // Use "hg pull" followed by "hg update" to update the working tree.
  {
  const char* hg = this->CommandLineTool.c_str();
  const char* hg_pull[] = {hg, "pull","-v", 0};
  OutputLogger out(this->Log, "pull-out> ");
  OutputLogger err(this->Log, "pull-err> ");
  this->RunChild(&hg_pull[0], &out, &err);
  }

  // TODO: if(this->CTest->GetTestModel() == cmCTest::NIGHTLY)

  std::vector<char const*> hg_update;
  hg_update.push_back(this->CommandLineTool.c_str());
  hg_update.push_back("update");
  hg_update.push_back("-v");

  // Add user-specified update options.
  std::string opts = this->CTest->GetCTestConfiguration("UpdateOptions");
  if(opts.empty())
    {
    opts = this->CTest->GetCTestConfiguration("HGUpdateOptions");
    }
  std::vector<cmStdString> args = cmSystemTools::ParseArguments(opts.c_str());
  for(std::vector<cmStdString>::const_iterator ai = args.begin();
      ai != args.end(); ++ai)
    {
    hg_update.push_back(ai->c_str());
    }

  // Sentinel argument.
  hg_update.push_back(0);

  OutputLogger out(this->Log, "update-out> ");
  OutputLogger err(this->Log, "update-err> ");
  return this->RunUpdateCommand(&hg_update[0], &out, &err);
}

//----------------------------------------------------------------------------
class cmCTestHG::LogParser: public cmCTestVC::OutputLogger,
                            private cmXMLParser
{
public:
  LogParser(cmCTestHG* hg, const char* prefix):
    OutputLogger(hg->Log, prefix), HG(hg) { this->InitializeParser(); }
  ~LogParser() { this->CleanupParser(); }
private:
  cmCTestHG* HG;

  typedef cmCTestHG::Revision Revision;
  typedef cmCTestHG::Change Change;
  Revision Rev;
  std::vector<Change> Changes;
  Change CurChange;
  std::vector<char> CData;

  virtual bool ProcessChunk(const char* data, int length)
    {
    this->OutputLogger::ProcessChunk(data, length);
    this->ParseChunk(data, length);
    return true;
    }

  virtual void StartElement(const char* name, const char** atts)
    {
    this->CData.clear();
    if(strcmp(name, "logentry") == 0)
      {
      this->Rev = Revision();
      if(const char* rev = this->FindAttribute(atts, "revision"))
        {
        this->Rev.Rev = rev;
        }
      this->Changes.clear();
      }
    }

  virtual void CharacterDataHandler(const char* data, int length)
    {
    this->CData.insert(this->CData.end(), data, data+length);
    }

  virtual void EndElement(const char* name)
    {
    if(strcmp(name, "logentry") == 0)
      {
      this->HG->DoRevision(this->Rev, this->Changes);
      }
    else if(strcmp(name, "author") == 0 && !this->CData.empty())
      {
      this->Rev.Author.assign(&this->CData[0], this->CData.size());
      }
    else if ( strcmp(name, "email") == 0 && !this->CData.empty())
      {
      // this->Rev.Email.assign(&this->CData[0], this->CData.size());
      }
    else if(strcmp(name, "date") == 0 && !this->CData.empty())
      {
      this->Rev.Date.assign(&this->CData[0], this->CData.size());
      }
    else if(strcmp(name, "msg") == 0 && !this->CData.empty())
      {
      this->Rev.Log.assign(&this->CData[0], this->CData.size());
      }
    else if(strcmp(name, "files") == 0 && !this->CData.empty())
      {
      std::vector<std::string> paths = this->SplitCData();
      for(unsigned int i = 0; i < paths.size(); ++i)
        {
        // Updated by default, will be modified using file_adds and
        // file_dels.
        this->CurChange = Change('U');
        this->CurChange.Path = paths[i];
        this->Changes.push_back(this->CurChange);
        }
      }
    else if(strcmp(name, "file_adds") == 0 && !this->CData.empty())
      {
      std::string added_paths(this->CData.begin(), this->CData.end());
      for(unsigned int i = 0; i < this->Changes.size(); ++i)
        {
        if(added_paths.find(this->Changes[i].Path) != std::string::npos)
          {
          this->Changes[i].Action = 'A';
          }
        }
      }
     else if(strcmp(name, "file_dels") == 0 && !this->CData.empty())
      {
      std::string added_paths(this->CData.begin(), this->CData.end());
      for(unsigned int i = 0; i < this->Changes.size(); ++i)
        {
        if(added_paths.find(this->Changes[i].Path) != std::string::npos)
          {
          this->Changes[i].Action = 'D';
          }
        }
      }
    this->CData.clear();
    }

  std::vector<std::string> SplitCData()
    {
    std::vector<std::string> output;
    std::string currPath;
    for(unsigned int i=0; i < this->CData.size(); ++i)
      {
      if(this->CData[i] != ' ')
        {
        currPath.push_back(this->CData[i]);
        }
      else
        {
        output.push_back(currPath);
        currPath.erase();
        }
      }
    output.push_back(currPath);
    return output;
    }

  virtual void ReportError(int, int, const char* msg)
    {
    this->HG->Log << "Error parsing hg log xml: " << msg << "\n";
    }
};

//----------------------------------------------------------------------------
void cmCTestHG::LoadRevisions()
{
  // Use 'hg log' to get revisions in a xml format.
  //
  // TODO: This should use plumbing or python code to be more precise.
  // The "list of strings" templates like {files} will not work when
  // the project has spaces in the path.  Also, they may not have
  // proper XML escapes.
  std::string range = this->OldRevision + ":" + this->NewRevision;
  const char* hg = this->CommandLineTool.c_str();
  const char* hgXMLTemplate =
    "<logentry\n"
    "   revision=\"{node|short}\">\n"
    "  <author>{author|person}</author>\n"
    "  <email>{author|email}</email>\n"
    "  <date>{date|isodate}</date>\n"
    "  <msg>{desc}</msg>\n"
    "  <files>{files}</files>\n"
    "  <file_adds>{file_adds}</file_adds>\n"
    "  <file_dels>{file_dels}</file_dels>\n"
    "</logentry>\n";
  const char* hg_log[] = {hg, "log","--removed", "-r", range.c_str(),
                          "--template", hgXMLTemplate, 0};

  LogParser out(this, "log-out> ");
  out.Process("<?xml version=\"1.0\"?>\n"
              "<log>\n");
  OutputLogger err(this->Log, "log-err> ");
  this->RunChild(hg_log, &out, &err);
  out.Process("</log>\n");
}

//----------------------------------------------------------------------------
void cmCTestHG::LoadModifications()
{
  // Use 'hg status' to get modified files.
  const char* hg = this->CommandLineTool.c_str();
  const char* hg_status[] = {hg, "status", 0};
  StatusParser out(this, "status-out> ");
  OutputLogger err(this->Log, "status-err> ");
  this->RunChild(hg_status, &out, &err);
}
