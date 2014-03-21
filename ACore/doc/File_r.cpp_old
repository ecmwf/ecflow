

File_r::File_r(const std::string& file_name) : file_name_(file_name),
   fp_( fopen(file_name.c_str(),"r") ),
   good_(true)
{
}

File_r::~File_r()
{
   fclose(fp_);
}

bool File_r::ok() const
{
   if (fp_ == 0) return false;
   return true;
}

bool File_r::good() const
{
   if (fp_ == 0) return false;
   return good_;
}

void File_r::getline(std::string& line)
{
   line.erase();
   char buffer[4096] =  { 0 } ;
   int c = 0;
   int i = 0;
   while ( (c = fgetc(fp_)) != EOF) {
      if (c == 10 /* new line */) {
         buffer[i] = '\0';
         break;
      }
      else {
         buffer[i] = c;
      }
      i++;
   }
   if (c == EOF) {
      good_ = false;
   }
   line = buffer;
}

}
