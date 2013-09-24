from pygments.lexers.agile import PythonLexer
from pygments.token import Name, Keyword

def qw(spaced_str): 
  return spaced_str.split()

class DefLexer(PythonLexer):
    # EXTRA_KEYWORDS = ['foo', 'bar', 'foobar', 'barfoo', 'spam', 'eggs']
    EXTRA_KEYWORDS = qw("EOF abort action autocancel   automigrate  autorestore"
                        "clock complete cron date day defstatus"+   
                        "edit endfamily endsuite endtask event extern"+      
                        "family inlimit label late limit meter"+
                        "owner repeat suite task text time"+
                        "today trigger")

    def get_tokens_unprocessed(self, text):
        for index, token, value in DefLexer.get_tokens_unprocessed(self, text):
            if token is Name and value in self.EXTRA_KEYWORDS:
                yield index, Keyword.Pseudo, value
            else:
                yield index, token, value
