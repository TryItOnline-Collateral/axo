#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <cstdlib>



using namespace std;

typedef int stack_item;



class Axo2Interpreter {
    vector<string> m_program;
    istream *m_input;
    ostream *m_output;
    int m_x,m_y,m_dx,m_dy;
    int m_width,m_height;
    deque<stack_item> m_stack;
    enum {mode_command,mode_string,mode_raw} m_mode;
    stack_item m_mem[2048];
    bool m_running;
    
    stack_item m_reg_a,m_reg_b;
    
    stack_item peak() {
        if(m_stack.empty())
            return 0;
        return m_stack.back();
    }
    
    stack_item pop() {
        if(m_stack.empty())
            return 0;
        stack_item rv = m_stack.back();
        m_stack.pop_back();
        return rv;
    }
    
    stack_item dequeue() {
        if(m_stack.empty())
            return 0;
        stack_item rv = m_stack.front();
        m_stack.pop_front();
        return rv;
    }
    
    void push(stack_item a) {
        m_stack.push_back(a);
    }
    
    void enqueue(stack_item a) {
        m_stack.push_front(a);
    }
    
    void clear_stack() {
        m_stack.clear();
    }
    
    bool outside() {
        return m_x < 0 || m_x >= m_width || m_y < 0 || m_y >= m_height;
    }
    
    char current_command() {
        return m_program[m_y][m_x];
    }
    
    void set_direction(int num) {
        switch(num&3) {
        case 0:
            m_dx= 0;m_dy=-1;
            return;
        case 1:
            m_dx=-1;m_dy= 0;
            return;
        case 2:
            m_dx= 1;m_dy= 0;
            return;
        case 3:
            m_dx= 0;m_dy= 1;
            return;
        }
    }
    
    void move() {
        m_x+=m_dx;
        m_y+=m_dy;
    }
    
    
    inline void execute_command(char command);
public:
    Axo2Interpreter(istream &, istream &, ostream &);
    void set_input(istream &input) {m_input = &input;}
    void set_output(ostream &output) {m_output = &output;}
    bool is_running() {return m_running;}
    
    void step() {
        if(!outside()) {
            execute_command(current_command());
            move();
            //cerr << "x: " << m_x << " y: " << m_y << endl; 
        }
    }
    
    void run() {
        while(m_running) 
            step();
    }
};

Axo2Interpreter::Axo2Interpreter(istream &program_file, istream &input = cin, ostream &output = cout) {
    string rline;
    m_input = &input;
    m_output = &output;
    m_x = m_y = 0;
    m_dx = 1;
    m_dy = 0;
    m_height = 0;
    m_width = 0;
    m_mode = mode_command;
    m_reg_a = m_reg_b = 0;
    m_running = true;
    while(getline(program_file,rline,'\n') && rline != "__END__") {
        m_height++;
        m_program.push_back(rline);
        if(rline.size()>m_width)
            m_width = rline.size();
    }
    for(vector<string>::iterator cline = m_program.begin(); cline!=m_program.end();++cline) {
        (*cline).append(m_width-cline->size(),' ');
    }
    srand(time(NULL));
}

inline void Axo2Interpreter::execute_command(char command) {
    stack_item a,b;
    int tmpd;
    if(m_mode != mode_raw) {
        switch(command) {
        case '^':
            m_dx= 0;m_dy=-1;
            return;
        case '<':
            m_dx=-1;m_dy= 0;
            return;
        case '>':
            m_dx= 1;m_dy= 0;
            return;
        case '%':
            m_dx= 0;m_dy= 1;
            return;
        case '+':
            push(pop()+pop());
            return;
        case '-':
            a=pop();
            push(pop()-a);
            return;
        case '*':
            push(pop()*pop());
            return;
        case '/':
            a=pop();
            b=pop();
            push(b/a);
            push(b%a);
            return;
        case '\\':
            m_running = false;
            return;
        }
    }
    if(m_mode == mode_command) {
        switch(command) {
        case '&':
            enqueue(pop());
            return;
        case '|':
            push(dequeue());
            return;
        case '$':
            set_direction(pop());
            return;
        case '#':
            if(!pop())
                move();
            return;
        case '[':
            push(peak());
            return;
        case ']':
            pop();
            return;
        case '(':
          
            m_output->put((char)(pop()&0xFF));
            m_output->flush();
            return;
        case ')':
            char inchar;
            m_input->get(inchar);
            if(m_input->eof())
                push(-1);
            else
                push(((stack_item)inchar)&0xFF);
            return;
        case '{':
            (*m_output) << pop();
            return;
        case '}':
            char ic;
            stack_item cnum;
            bool neg;
            cnum = 0;
            
            do m_input->get(ic);
            while(ic < '0' || ic > '9' && ic != '-');
            
            neg = false;
            if(ic == '-') {
                neg = true;
                m_input->get(ic);
            }
            
            while(ic >= '0' && ic <= '9') {
                cnum = cnum * 10 + ic-'0';
                m_input->get(ic);
            }
            
            while(ic != '\n')
                m_input->get(ic);
            
            if(neg)
                cnum = -cnum;
            
            push(cnum);

            return;
        case '"':
            m_mode = mode_string;
            return;
        case '\'':
            m_mode = mode_raw;
            return;
        case ':':
            m_reg_a = pop();
            return;
        case ';':
            push(m_reg_a);
            return;
        case '.':
            m_reg_b = pop();
            return;
        case ',':
            push(m_reg_b);
            return;
        case '?':
            set_direction(random());
            return;
        case '_':
            m_x = -1;
            m_y = 0;
            m_dx = 1;
            m_dy = 0;
            return;
        case '@':
            clear_stack();
            return;
        case '!':
            tmpd = m_dx;
            m_dx=-m_dy;
            m_dy=tmpd;
            return;
        case '=':
            a = pop();
            b = pop();
            push(m_mem[a&2047]);
            m_mem[a&2047]=b;
            return;
        case '~':
            cerr << "\n[ ";
            bool first;
            first = true;
            for(deque<stack_item>::const_iterator i = m_stack.begin();i != m_stack.end();i++){
                if(!first)
                    cerr << " | ";
                cerr << *i;
                first = false;
            }
            cerr << " ] a:" << m_reg_a << " b:" << m_reg_b << " x:" << m_x << " y:" << m_y << endl;
            
            return;
        default:
            if(command >= '0' and command <= '9'){
                push(command-'0');
                return;
            }
        }
    }
    if(m_mode == mode_string) {
        if(command == '"')
            m_mode = mode_command;
        else if(command == '$')
            push(10);
        else
            push(((stack_item)command)&0xFF);
    }
    else if(m_mode == mode_raw) {
        if(command == '\'')
            m_mode = mode_command;
        else
            push(((stack_item)command)&0xFF);
    }
}

// quick and dirty cli 

void usage() {
    cerr << "Usage: axopp <sourcefile>" << endl;
    exit(1);
}

int main(int argc, char * const argv[]) {
    if(argc != 2)
        usage();


    if(argv[1][0] == '-' && argv[1][1] == 0) {
        Axo2Interpreter i(cin);
        i.run();
    }
    else {
        ifstream inf;
        inf.open(argv[1],ios::in);
        if(!inf.is_open()) {
            cerr << "Couldn't open sourcefile." << endl;
            return 1;
        }
        Axo2Interpreter i(inf);
        i.run();
        inf.close();
    }
    

    
  return 0;
}

