#include "src/nbt_loader.h"

#include "glog/logging.h"

bool getshort(int a, int i, int* dx, int* dy, int* dz) {
  *dx = *dy = *dz = 0;
  
  if (a == 0b01) *dx = i - 5;
  else if (a == 0b10) *dy = i - 5;
  else if (a == 0b11) *dz = i - 5;
  else return false;

  return true;
}

bool getlong(int a, int i, int* dx, int* dy, int* dz) {
  *dx = *dy = *dz = 0;
  
  if (a == 0b01) *dx = i - 15;
  else if (a == 0b10) *dy = i - 15;
  else if (a == 0b11) *dz = i - 15;
  else return false;

  return true;
}

bool getnearcoordinate(int nd, int* dx, int* dy, int* dz) {
  for (*dx = -1; *dx <= 1; ++*dx) {
    for (*dy = -1; *dy <= 1; ++*dy) {
      for (*dz = -1; *dz <= 1; ++*dz) {
        if ((*dx + 1) * 9 + (*dy + 1) * 3 + (*dz + 1) == nd) {
          return true;
        }
      }
    }
  }

  return false;
}

std::string binary(int x) {
  std::string a;
  for (int i = 7; i >= 0; --i) {
    a += ((x >> i) & 1) + '0';
  }
  return a;
}

int parse_command(const std::string& nbt_content, int i, int* nanobot_num, std::ostringstream* ss, Json::Value* command) {

  switch (nbt_content[i]) {
  case kHALT:
    (*ss) << "Halt" << std::endl;
    (*command)["command"] = "Halt";
    return 1;

  case kWAIT:
    (*ss) << "Wait" << std::endl;
    (*command)["command"] = "Wait";
    return 1;

  case kFLIP:
    (*ss) << "Flip" << std::endl;
    (*command)["command"] = "Flip";
    return 1;

  default:
    //
    break;
  }

  if ((nbt_content[i] & 0b1111) == 0b0100) {
    // SMove
    int llda = (nbt_content[i] >> 4) & 0b11;
    int lldi = nbt_content[i + 1] & 0b11111;
    int dx = 0, dy = 0, dz = 0;
    LOG_IF(FATAL, !getlong(llda, lldi, &dx, &dy, &dz))
      << "encoding error";

    (*command)["command"] = "SMove";
    (*command)["dx"] = dx;
    (*command)["dy"] = dy;
    (*command)["dz"] = dz;
    
    (*ss) << "SMove <" << dx << ", " << dy << ", " << dz << ">" << std::endl;
    return 2;
  }

  if ((nbt_content[i] & 0b1111) == 0b1100) {
    // LMove
    int sid2a = (nbt_content[i] >> 6) & 0b11;
    int sid1a = (nbt_content[i] >> 4) & 0b11;
    int sid2i = (nbt_content[i + 1] >> 4) & 0b1111;
    int sid1i = nbt_content[i + 1] & 0b1111;
      
    int dx1 = 0, dy1 = 0, dz1 = 0;
    int dx2 = 0, dy2 = 0, dz2 = 0;
    LOG_IF(FATAL, !getshort(sid1a, sid1i, &dx1, &dy1, &dz1))
      << "encoding error";

    LOG_IF(FATAL, !getshort(sid2a, sid2i, &dx2, &dy2, &dz2))
      << "encoding error" << std::endl;

    (*command)["command"] = "LMove";
    (*command)["dx1"] = dx1;
    (*command)["dy1"] = dy1;
    (*command)["dz1"] = dz1;
    (*command)["dx2"] = dx2;
    (*command)["dy2"] = dy2;
    (*command)["dz2"] = dz2;

    (*ss) << "LMove <" << dx1 << ", " << dy1 << ", " << dz1 << "> <" 
       << dx2 << ", " << dy2 << ", " << dz2 << ">"
       << std::endl;
    return 2;
  }
    
  if ((nbt_content[i] & 0b111) == 0b111) {
    // FusionP
    --*nanobot_num;
    int nd = (nbt_content[i] >> 3) & 0b11111;
    int dx, dy, dz;

    LOG_IF(FATAL, !getnearcoordinate(nd, &dx, &dy, &dz))
      << "encoding error";
      
    (*ss) << "FusionP <" << dx << ", " << dy << ", " << dz << ">"  << std::endl;

    (*command)["command"] = "FusionP";
    (*command)["dx"] = dx;
    (*command)["dy"] = dy;
    (*command)["dz"] = dz;
    
    return 1;
  }

  if ((nbt_content[i] & 0b111) == 0b110) {
    // FusionS
    int nd = (nbt_content[i] >> 3) & 0b11111;
    int dx, dy, dz;
    LOG_IF(FATAL, !getnearcoordinate(nd, &dx, &dy, &dz))
      << "encoding error" << std::endl;

    (*ss) << "FusionS <" << dx << ", " << dy << ", " << dz << ">"  << std::endl;

    (*command)["command"] = "FusionS";
    (*command)["dx"] = dx;
    (*command)["dy"] = dy;
    (*command)["dz"] = dz;

    return 1;
  }

  if ((nbt_content[i] & 0b111) == 0b101) {
    // Fission
    ++*nanobot_num;
    int nd = (nbt_content[i] >> 3) & 0b11111;
    int m = nbt_content[i + 1];
    int dx, dy, dz;
    LOG_IF(FATAL, !getnearcoordinate(nd, &dx, &dy, &dz))
      << "encoding error";

    (*ss) << "Fission <" << dx << ", " << dy << ", " << dz << "> "  << m << std::endl;

    (*command)["command"] = "Fission";
    (*command)["dx"] = dx;
    (*command)["dy"] = dy;
    (*command)["dz"] = dz;
    (*command)["m"] = m;

    return 2;
  }

  if ((nbt_content[i] & 0b111) == 0b011) {
    // Fill
    int nd = (nbt_content[i] >> 3) & 0b11111;
    int dx, dy, dz;
    LOG_IF(FATAL, !getnearcoordinate(nd, &dx, &dy, &dz))
      << "encoding error" << std::endl;

    (*ss) << "Fill <" << dx << ", " << dy << ", " << dz << ">"  << std::endl;

    (*command)["command"] = "Fill";
    (*command)["dx"] = dx;
    (*command)["dy"] = dy;
    (*command)["dz"] = dz;
    return 1;
  }

  if ((nbt_content[i] & 0b111) == 0b010) {
    // Void
    int nd = (nbt_content[i] >> 3) & 0b11111;
    int dx, dy, dz;
    LOG_IF(FATAL, !getnearcoordinate(nd, &dx, &dy, &dz))
      << "encoding error" << std::endl;

    (*ss) << "Void <" << dx << ", " << dy << ", " << dz << ">"  << std::endl;

    (*command)["command"] = "Void";
    (*command)["dx"] = dx;
    (*command)["dy"] = dy;
    (*command)["dz"] = dz;
    return 1;
  }

  if ((nbt_content[i] & 0b111) == 0b000) {
    // GVoid
    int nd = (nbt_content[i] >> 3) & 0b11111;
    int dx1, dy1, dz1;
    LOG_IF(FATAL, !getnearcoordinate(nd, &dx1, &dy1, &dz1))
      << "encoding error" << std::endl;

    int dx2 = static_cast<int>(nbt_content[i + 1]) - 30;
    int dy2 = static_cast<int>(nbt_content[i + 2]) - 30;
    int dz2 = static_cast<int>(nbt_content[i + 3]) - 30;

    
    (*ss) << "GVoid <" << dx1 << ", " << dy1 << ", " << dz1 << ">"  
          << " <" << dx2 << ", " << dy2 << ", " << dz2 << ">"
          << std::endl;

    (*command)["command"] = "GVoid";
    (*command)["dx1"] = dx1;
    (*command)["dy1"] = dy1;
    (*command)["dz1"] = dz1;
    (*command)["dx2"] = dx2;
    (*command)["dy2"] = dy2;
    (*command)["dz2"] = dz2;
    return 4;
  }

  if ((nbt_content[i] & 0b111) == 0b001) {
    // GFill
    int nd = (nbt_content[i] >> 3) & 0b11111;
    int dx1, dy1, dz1;
    LOG_IF(FATAL, !getnearcoordinate(nd, &dx1, &dy1, &dz1))
      << "encoding error" << std::endl;

    int dx2 = static_cast<int>(nbt_content[i + 1]) - 30;
    int dy2 = static_cast<int>(nbt_content[i + 2]) - 30;
    int dz2 = static_cast<int>(nbt_content[i + 3]) - 30;

    (*ss) << "GFill <" << dx1 << ", " << dy1 << ", " << dz1 << ">"  
          << " <" << dx2 << ", " << dy2 << ", " << dz2 << ">"
          << std::endl;

    (*command)["command"] = "GFill";
    (*command)["dx1"] = dx1;
    (*command)["dy1"] = dy1;
    (*command)["dz1"] = dz1;
    (*command)["dx2"] = dx2;
    (*command)["dy2"] = dy2;
    (*command)["dz2"] = dz2;
    return 4;
  }

  LOG(FATAL) << "unknown command? " << binary(nbt_content[i]);
}

