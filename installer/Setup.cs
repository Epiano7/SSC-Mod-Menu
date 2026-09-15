using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Windows.Forms;

namespace SSCMods.Setup {
    public static class Release {
        public static Package Package() {
#if MENU_ALPHA
            var assembly=Assembly.GetExecutingAssembly();
            var package=new Package {RuntimeValidated=true,GameSize=15221248,
                GameHash="34D8809E646C36E595FDB9DF072E09B31EA35108DEFF3B32EF40451695D05307"};
            foreach(var name in new[]{"opengl32.dll","runtime.dll"}) {
                using(var stream=assembly.GetManifestResourceStream(name))
                using(var bytes=new MemoryStream()) {
                    if(stream==null) throw new IOException("Embedded runtime is missing.");
                    stream.CopyTo(bytes);
                    package.Files.Add(new Payload {Path=name=="runtime.dll"?"SSCMods/runtime.dll":name,Bytes=bytes.ToArray()});
                }
            }
            package.Files.Add(new Payload {Path="SSCMods/Uninstall.exe",Bytes=File.ReadAllBytes(assembly.Location)});
            return package;
#else
            return new Package {RuntimeValidated=false};
#endif
        }
    }
    static class Theme {
        public static readonly Color Background=Color.FromArgb(15,28,46), Panel=Color.FromArgb(27,47,73),
            Raised=Color.FromArgb(38,64,96), Blue=Color.FromArgb(31,117,215), Cyan=Color.FromArgb(77,196,255),
            Ink=Color.FromArgb(229,237,250), Muted=Color.FromArgb(164,187,217);
    }
    sealed class CutButton : Button {
        public bool Selected;
        bool hover;
        public CutButton(){FlatStyle=FlatStyle.Flat;FlatAppearance.BorderSize=0;Cursor=Cursors.Hand;Font=new Font("Segoe UI",10,FontStyle.Bold);SetStyle(ControlStyles.UserPaint|ControlStyles.AllPaintingInWmPaint|ControlStyles.OptimizedDoubleBuffer,true);}
        protected override void OnMouseEnter(EventArgs e){hover=true;Invalidate();base.OnMouseEnter(e);}
        protected override void OnMouseLeave(EventArgs e){hover=false;Invalidate();base.OnMouseLeave(e);}
        protected override void OnPaint(PaintEventArgs e){
            e.Graphics.Clear(Parent.BackColor);int w=Width-1,h=Height-1,c=8;
            var points=new[]{new Point(c,0),new Point(w-c,0),new Point(w,c),new Point(w,h-c),new Point(w-c,h),new Point(c,h),new Point(0,h-c),new Point(0,c)};
            Color fill=!Enabled?Theme.Panel:Selected?Theme.Blue:hover?Color.FromArgb(51,85,126):Theme.Raised;
            using(var brush=new SolidBrush(fill))e.Graphics.FillPolygon(brush,points);
            using(var pen=new Pen(Selected?Theme.Cyan:Theme.Raised))e.Graphics.DrawLine(pen,c,0,w-c,0);
            TextRenderer.DrawText(e.Graphics,Text,Font,ClientRectangle,Enabled?Theme.Ink:Theme.Muted,TextFormatFlags.HorizontalCenter|TextFormatFlags.VerticalCenter|TextFormatFlags.EndEllipsis);
            if(Focused&&ShowFocusCues)ControlPaint.DrawFocusRectangle(e.Graphics,Rectangle.Inflate(ClientRectangle,-5,-5),Theme.Cyan,fill);
        }
    }
    public sealed class SetupWindow : Form {
        readonly TextBox folder=new TextBox(),details=new TextBox();
        readonly Label status=new Label(),heading=new Label(),summary=new Label();
        readonly CutButton action=new CutButton(),installTab=new CutButton(),removeTab=new CutButton(),browse=new CutButton(),check=new CutButton();
        readonly Package package;
        bool uninstall,busy;
        public bool Busy {get{return busy;}}
        public SetupWindow(string root,bool remove):this(root,remove,Release.Package()){}
        internal SetupWindow(string root,bool remove,Package payload){
            package=payload;uninstall=remove;Text="SSC Mods - Setup";
            ClientSize=new Size(820,590);FormBorderStyle=FormBorderStyle.FixedSingle;MaximizeBox=false;
            AutoScaleMode=AutoScaleMode.Dpi;Font=new Font("Segoe UI",10);BackColor=Theme.Background;ForeColor=Theme.Ink;
            StartPosition=FormStartPosition.CenterScreen;DoubleBuffered=true;
            var header=new Panel{BackColor=Theme.Panel};header.SetBounds(0,0,820,92);Controls.Add(header);
            header.Controls.Add(new Label{Text="SSC MODS",Font=new Font("Segoe UI",23,FontStyle.Bold),ForeColor=Theme.Ink,Left=26,Top=15,Width=500,Height=42});
            header.Controls.Add(new Label{Text="CLIENT SETUP",ForeColor=Theme.Muted,Left=28,Top=59,Width=300,Height=24});
            var rule=new Panel{BackColor=Theme.Blue};rule.SetBounds(0,90,820,2);Controls.Add(rule);rule.BringToFront();
            installTab.Name="installTab";installTab.Text="INSTALL / UPDATE";installTab.SetBounds(26,114,224,42);installTab.Click+=(s,e)=>SelectMode(false);Controls.Add(installTab);
            removeTab.Name="removeTab";removeTab.Text="UNINSTALL";removeTab.SetBounds(264,114,176,42);removeTab.Click+=(s,e)=>SelectMode(true);Controls.Add(removeTab);
            heading.SetBounds(26,179,760,34);heading.Font=new Font("Segoe UI",16,FontStyle.Bold);Controls.Add(heading);
            summary.SetBounds(28,218,758,30);summary.ForeColor=Theme.Muted;Controls.Add(summary);
            Controls.Add(new Label{Text="GAME FOLDER",ForeColor=Theme.Muted,Left=28,Top=267,Width=300,Height=24});
            var pathPanel=new Panel{BackColor=Theme.Panel};pathPanel.SetBounds(28,295,620,40);Controls.Add(pathPanel);
            folder.Name="folder";folder.BorderStyle=BorderStyle.None;folder.BackColor=Theme.Panel;folder.ForeColor=Theme.Ink;folder.SetBounds(12,9,596,25);folder.Text=root??"";pathPanel.Controls.Add(folder);
            folder.AccessibleName="Skillshot City game folder";
            browse.Name="browse";browse.Text="BROWSE";browse.SetBounds(662,295,130,40);browse.Click+=(s,e)=>{
                using(var dialog=new FolderBrowserDialog()){dialog.Description="Choose the Skillshot City game folder";dialog.SelectedPath=folder.Text;if(dialog.ShowDialog(this)==DialogResult.OK){folder.Text=dialog.SelectedPath;Check();}}
            };Controls.Add(browse);
            status.Name="status";status.SetBounds(28,354,762,50);status.ForeColor=Theme.Cyan;Controls.Add(status);
            details.Name="details";details.SetBounds(28,407,764,88);details.Multiline=true;details.ReadOnly=true;details.BorderStyle=BorderStyle.None;details.BackColor=Theme.Background;details.ForeColor=Theme.Muted;details.ScrollBars=ScrollBars.None;details.TabStop=false;Controls.Add(details);
            var footer=new Panel{BackColor=Theme.Raised};footer.SetBounds(26,512,768,1);Controls.Add(footer);
            check.Name="check";check.Text="CHECK AGAIN";check.SetBounds(28,534,154,40);check.Click+=(s,e)=>Check();Controls.Add(check);
            action.Name="action";action.Selected=true;action.SetBounds(440,534,208,40);action.Click+=(s,e)=>Execute();Controls.Add(action);
            var close=new CutButton{Name="close",Text="CLOSE"};close.SetBounds(662,534,130,40);close.Click+=(s,e)=>Close();Controls.Add(close);
            AcceptButton=action;CancelButton=close;
            folder.TextChanged+=(s,e)=>{action.Enabled=false;status.Text="Check this folder to continue.";};
            FormClosing+=(s,e)=>{if(busy)e.Cancel=true;};
            SetModeText();Shown+=(s,e)=>Check();
        }
        void SetModeText(){
            installTab.Selected=!uninstall;removeTab.Selected=uninstall;installTab.Invalidate();removeTab.Invalidate();
            heading.Text=uninstall?"Uninstall SSC Mods":"Set up your client";
            summary.Text=uninstall?"Remove the mod from your selected game folder.":"Install or update SSC Mods, then launch through Steam.";
            details.Text=uninstall?"Your saved settings will stay available. Changed or unrelated files are kept.":"SOUND REPLACER    /    COSMETICS\r\nDISCORD PRESENCE    /    HUD EDITOR\r\nRight Shift opens the menu in game.";
            action.Text=uninstall?"UNINSTALL":"INSTALL";
        }
        void SelectMode(bool remove){if(busy)return;uninstall=remove;SetModeText();Check();}
        void SetBusy(bool value){busy=value;folder.Enabled=browse.Enabled=check.Enabled=installTab.Enabled=removeTab.Enabled=!value;action.Enabled=false;UseWaitCursor=value;}
        async void Check(){
            if(busy)return;SetBusy(true);status.ForeColor=Theme.Cyan;status.Text="Checking game folder...";
            try{
                string path=folder.Text;
                if(String.IsNullOrWhiteSpace(path)){path=await System.Threading.Tasks.Task.Run(()=>Engine.Discover());folder.Text=path;}
                bool remove=uninstall;
                var result=await System.Threading.Tasks.Task.Run(()=>{
                    if(remove)return Engine.InspectRemoval(path);
                    return File.Exists(Path.Combine(path,Engine.ManifestName))?Engine.InspectUpdate(path,package):Engine.Inspect(path,package);
                });
                bool updating=File.Exists(Path.Combine(path,Engine.ManifestName));
                SetBusy(false);action.Text=remove?"UNINSTALL":updating?"UPDATE":"INSTALL";
                status.Text=result.CanInstall?(remove?"Ready to uninstall.":updating?"SSC Mods is installed. Ready to update.":"Ready to install."):result.Message;
                action.Enabled=result.CanInstall;
            }catch(Exception error){SetBusy(false);status.ForeColor=Color.FromArgb(255,196,99);status.Text=error.Message;}
        }
        public void RefreshInspection(){Check();}
        async void Execute(){
            if(busy||!action.Enabled)return;string path=folder.Text;bool remove=uninstall;
            SetBusy(true);status.ForeColor=Theme.Cyan;status.Text=remove?"Uninstalling...":"Installing...";
            try{
                if(remove){
                    var retained=await System.Threading.Tasks.Task.Run(()=>Engine.Uninstall(path));
                    status.Text=retained.Count==0?"SSC Mods uninstalled.":"Some changed files were kept.";
                    details.Text=retained.Count==0?"Your game is ready to launch through Steam. Saved mod settings are kept.":"Kept files:\r\n"+String.Join("\r\n",retained);
                }else{
                    await System.Threading.Tasks.Task.Run(()=>Engine.InstallOrUpdate(path,package));
                    status.Text="SSC Mods is ready.";details.Text="Launch Skillshot City through Steam. Press Right Shift to open the menu.";
                }
                SetBusy(false);
            }catch(Exception error){SetBusy(false);status.ForeColor=Color.FromArgb(255,196,99);status.Text="Could not complete this action.";details.Text=error.Message;}
        }
    }
    public static class Program {
        [STAThread]
        public static int Main(string[] args) {
            Application.EnableVisualStyles(); Application.SetCompatibleTextRenderingDefault(false);
            try {
                if(args.Length==2&&args[0]=="--apply") {Engine.InstallOrUpdate(args[1],Release.Package());return 0;}
                string exe=Assembly.GetExecutingAssembly().Location;
                // The installed uninstaller runs from a private staging folder so the game-side EXE can be removed.
                if(Path.GetFileName(exe).Equals("Uninstall.exe",StringComparison.OrdinalIgnoreCase)) {
                    string root=Directory.GetParent(Path.GetDirectoryName(exe)).FullName;
                    string temp=Path.Combine(Path.GetTempPath(),"SSCMods-Uninstall-"+Guid.NewGuid().ToString("N"));
                    Directory.CreateDirectory(temp);
                    string helper=Path.Combine(temp,"SSCMods-Setup.exe"); File.Copy(exe,helper,false);
                    Process.Start(new ProcessStartInfo(helper,"--uninstall \""+root+"\" --wait "+Process.GetCurrentProcess().Id) {UseShellExecute=true});
                    return 0;
                }
                bool remove=args.Length>=2 && args[0]=="--uninstall";
                if(args.Length==4 && remove && args[2]=="--wait") {
                    int pid;
                    if(!Int32.TryParse(args[3],out pid)) throw new IOException("Invalid uninstaller process identifier.");
                    try { using(var parent=Process.GetProcessById(pid)) if(!parent.WaitForExit(10000)) throw new IOException("Close the previous uninstaller and retry."); }
                    catch(ArgumentException) {}
                } else if(args.Length!=0 && !(remove&&args.Length==2)) throw new IOException("Usage: SSCMods-Setup.exe [--uninstall GAME_DIRECTORY]");
                Application.Run(new SetupWindow(remove?args[1]:null,remove)); return 0;
            } catch(Exception error) { if(args.Length>0&&args[0]=="--apply") {Console.Error.WriteLine(error);return 1;} MessageBox.Show(error.Message,"SSC Mods",MessageBoxButtons.OK,MessageBoxIcon.Error); return 1; }
        }
    }
}




