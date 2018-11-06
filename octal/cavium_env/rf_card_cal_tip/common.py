

def hit_continue(Prompt = 'Hit Enter key to continue'):
    raw_input(Prompt)
    
def disp_test_title(title):
    length = len(title)
    print ""
    print "*" * (length + 4)
    print "* " + title + " *"
    print "*" * (length + 4)
    print ""