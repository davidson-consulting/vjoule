import os
import subprocess

def get_template () :
    with open ("main.tex") as f :
        return f.read ()

def generate_image (f, template) :
    content = template.replace ("{{TEMPLATE}}", "../" + f)
    with open (".build/out.tex", "w") as fs:
        fs.write (content)


    p = subprocess.Popen (["pdflatex", "out.tex"], cwd = ".build/", stdout=subprocess.PIPE)
    p.stdout.read ()

    p = subprocess.Popen (["pdf2svg", "out.pdf", "../" + f[:-4] + ".svg"], cwd=".build/", stdout= subprocess.PIPE)
    p.stdout.read ()
    
    
    
def main () :
    template = get_template ()
    if (not os.path.exists (".build/")): 
        os.mkdir (".build/")
        
    for f in os.listdir('.') :
        if (f != "main.tex" and f[-3:] == "tex") : 
            generate_image (f, template)


if __name__ == "__main__" :
    main ()
