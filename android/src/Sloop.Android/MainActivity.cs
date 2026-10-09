using Android.App;
using Android.OS;
using Android.Graphics;
using Android.Views;
using Android.Widget;
using Sloop.Core;
using Sloop.Android.Services;

namespace Sloop.Android;

[Activity(Label = "SLOOP Mobile", MainLauncher = true, Exported = true)]
public sealed partial class MainActivity : Activity
{
    private readonly WorkstationState state = new();
    private LinearLayout content = null!;
    private Fm1Connection connection = null!;
    private TextView connectionStatus = null!;
    private Button connectButton = null!;
    private Button disconnectButton = null!;
    private Button simulateButton = null!;
    private readonly List<Button> trackButtons=[];
    private LinearLayout trackStrip=null!;
    private Workspace? renderedWorkspace;
    private int renderedTrack=-1;
    private int renderedSamplePage=-1;

    protected override void OnCreate(Bundle? savedInstanceState)
    {
        base.OnCreate(savedInstanceState);
        connection = Fm1Connection.Get(this);
        samples = SampleWorkspace.Get(this);
        editing=EditingWorkspace.Get(this);
        InitializeSoundAuditions();
        InitializeSessionIntegration();
        InitializeScenes();
        if (savedInstanceState is not null)
        {
            var workspace = savedInstanceState.GetInt("workspace", 0);
            if (Enum.IsDefined(typeof(Workspace), workspace)) state.Workspace = (Workspace)workspace;
            state.SelectTrack(Math.Clamp(savedInstanceState.GetInt("track", 0), 0, 3));
        }
        var root = new LinearLayout(this) { Orientation = Orientation.Vertical };
        root.SetPadding(Dp(12), Dp(24), Dp(12), Dp(12));
        root.SetOnApplyWindowInsetsListener(new SystemInsets(Dp(12)));
        root.SetBackgroundColor(Color.Rgb(18, 20, 24));
        var header = new LinearLayout(this) { Orientation = Orientation.Horizontal };
        header.SetGravity(GravityFlags.CenterVertical);
        header.AddView(Label("SLOOP", 22), new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WrapContent, 1));
        connectButton = new Button(this) { Text = "Connect" };
        connectButton.Click += (_, _) => ShowConnectionMenu();
        header.AddView(connectButton);
        StyleButton(connectButton);
        root.AddView(header);
        connectionStatus = Label(connection.Snapshot.Status, 12);
        connectionStatus.SetMaxLines(2);
        connectionStatus.Ellipsize = global::Android.Text.TextUtils.TruncateAt.End;
        connectionStatus.Click += (_, _) => ShowConnectionMenu();
        root.AddView(connectionStatus);
        disconnectButton = new Button(this);
        simulateButton = new Button(this);
        var tracks = new LinearLayout(this) { Orientation = Orientation.Horizontal };
        trackStrip=tracks;
        for (var i = 0; i < WorkstationState.TrackNames.Count; i++)
        {
            var index = i;
            var button = new Button(this) { Text = WorkstationState.TrackNames[i] };
            trackButtons.Add(button);
            button.Click += (_, _) => { CancelMusicalStarterPreview();StopCompositionAudition();StopPerformance();state.SelectTrack(index); ShowWorkspace(); };
            tracks.AddView(button, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WrapContent, 1));
        }
        root.AddView(tracks);
        var scroll = new ScrollView(this){MotionEventSplittingEnabled=true};
        content = new LinearLayout(this) { Orientation = Orientation.Vertical,MotionEventSplittingEnabled=true };
        scroll.AddView(content);
        root.AddView(scroll, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MatchParent, 0, 1));
        var tabs = new LinearLayout(this) { Orientation = Orientation.Horizontal };
        foreach (var workspace in Enum.GetValues<Workspace>())
        {
            var tab = new Button(this) { Text = workspace.ToString(),TextSize=11 };
            workspaceTabs.Add(tab);
            tab.SetSingleLine(true);
            tab.SetPadding(Dp(2),Dp(4),Dp(2),Dp(4));
            tab.SetMinWidth(0);
            tab.Click += (_, _) => { CancelMusicalStarterPreview();StopCompositionAudition();StopPerformance();state.Workspace = workspace; ShowWorkspace(); };
            tabs.AddView(tab,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WrapContent,1));
        }
        root.AddView(tabs);
        SetContentView(root);
        root.RequestApplyInsets();
        connection.Changed += OnConnectionChanged;
        samples.Changed += OnSamplesChanged;
        editing.Changed+=OnEditingChanged;
        connection.Performer.Failed+=OnPerformanceFailure;
        connection.HardwareOctaveChanged+=OnHardwareOctaveChanged;
        OnConnectionChanged();
    }

    private int Dp(int value) => (int)(value * (Resources?.DisplayMetrics?.Density ?? 1));

    private void ShowConnectionMenu()
    {
        new AlertDialog.Builder(this)!.SetTitle("MIDI connection")!
            .SetItems(new[] { "Connect SLOOP FM1", "Connect generic MIDI", "Try simulator (silent)", "Disconnect", "Connection details" }, async (_, args) =>
            {
                if (args.Which == 0) ChooseDevice();
                else if (args.Which == 1) ChooseDevice(generic:true);
                else if (args.Which == 2) await connection.ConnectSimulatedAsync();
                else if(args.Which==3)connection.Disconnect();
                else new AlertDialog.Builder(this)!.SetTitle("Connection details")!.SetMessage(connection.Snapshot.Status)!.SetPositiveButton("Close",(_,_)=>{})!.Show();
            })!.SetNegativeButton("Close", (_, _) => { })!.Show();
    }

    private void ChooseDevice(bool generic = false)
    {
        var candidates = connection.Candidates(generic);
        if (candidates.Count == 0)
        {
            new AlertDialog.Builder(this)!.SetTitle("No MIDI destination found")!
                .SetMessage(generic?"Connect a USB MIDI instrument or interface with a USB data cable. It must expose a MIDI input to Android.":"Connect the FM1 with a USB data cable. The phone must support USB host mode. Both MIDI ports must be available.")!
                .SetPositiveButton("OK", (_, _) => { })!.Show();
            return;
        }
        new AlertDialog.Builder(this)!.SetTitle(generic?"Choose a MIDI destination":"Choose the FM1 MIDI device")!
            .SetItems(candidates.Select(c => c.Name).ToArray(), async (_, args) =>
                await connection.ConnectAsync(candidates[args.Which],generic))!
            .SetNegativeButton("Cancel", (_, _) => { })!.Show();
    }

    private void OnConnectionChanged()
    {
        SynchronizeMidiInputDestination();
        StopPerformance();
        if(connection.Snapshot.Device is null){Array.Clear(hardwareSound);hardwareBaseline=null;}
        connectionStatus.Text = connection.Snapshot.Status;
        connectButton.Text = connection.Snapshot.Device is null&&!connection.Snapshot.IsGenericMidi ? "Connect" : "Device";
        connectButton.Enabled = !connection.Snapshot.Busy;
        disconnectButton.Enabled = connection.Snapshot.Busy || connection.Snapshot.Device is not null;
        simulateButton.Enabled = !connection.Snapshot.Busy;
        ShowWorkspace();
    }

    protected override void OnDestroy()
    {
        CancelMusicalStarterPreview();
        StopCompositionAudition();
        DisposeScenes();
        DisposeMidiInput();
        StopPerformance();connection.Performer.Failed-=OnPerformanceFailure;
        connection.HardwareOctaveChanged-=OnHardwareOctaveChanged;
        connection.Changed -= OnConnectionChanged;
        samples.Changed -= OnSamplesChanged;
        editing.Changed-=OnEditingChanged;
        samples.StopPreview();
        samples.CancelConversion();
        base.OnDestroy();
    }

    protected override void OnStop()
    {
        CancelMusicalStarterPreview();
        CloseMidiInput();
        StopCompositionAudition();
        StopPerformance();
        connection.StopPlaying();
        samples.StopPreview();
        samples.StopRecording("Interrupted: app left the foreground");
        recordingForeground = false;
        base.OnStop();
    }

    public override void OnWindowFocusChanged(bool hasFocus)
    {
        if(!hasFocus&&connection is not null)StopPerformance();
        base.OnWindowFocusChanged(hasFocus);
    }

    protected override void OnStart()
    {
        base.OnStart();
        recordingForeground = true;
    }

    protected override void OnSaveInstanceState(Bundle outState)
    {
        outState.PutInt("workspace", (int)state.Workspace);
        outState.PutInt("track", state.SelectedTrack);
        base.OnSaveInstanceState(outState);
    }

    private TextView Label(string text, float size = 18)
    {
        var label = new TextView(this) { Text = text, TextSize = size };
        label.SetTextColor(Color.Rgb(230, 233, 240));
        label.SetPadding(Dp(4), Dp(4), Dp(4), Dp(4));
        return label;
    }

    private void ShowWorkspace()
    {
        if(playingSurface is not null)StopPerformance();
        bool perform=state.Workspace==Workspace.Perform;
        trackStrip.Visibility=state.Workspace is Workspace.Sample or Workspace.Library?ViewStates.Gone:ViewStates.Visible;
        for(int i=0;i<trackButtons.Count;i++) StyleButton(trackButtons[i],i==state.SelectedTrack);
        for(int i=0;i<workspaceTabs.Count;i++) {
            StyleButton(workspaceTabs[i],i==(int)state.Workspace);
            workspaceTabs[i].TextSize=10;
            workspaceTabs[i].SetPadding(Dp(2),Dp(4),Dp(2),Dp(4));
        }
        var scroll=content.Parent as ScrollView;
        int position=renderedWorkspace==state.Workspace&&renderedTrack==state.SelectedTrack&&
            (state.Workspace!=Workspace.Sample||renderedSamplePage==samplePage)?scroll?.ScrollY??0:0;
        renderedWorkspace=state.Workspace; renderedTrack=state.SelectedTrack;
        renderedSamplePage=samplePage;
        scroll?.Post(()=>scroll.ScrollTo(0,position));
        content.RemoveAllViews();
        try {
        if(perform){AddPerformEditor();return;}
        
        if (state.Workspace == Workspace.Sound) {AddSoundEditor();return;}
        if (state.Workspace == Workspace.Sequence) {AddSequenceEditor();return;}
        if (state.Workspace == Workspace.Library) {AddLibraryEditor();return;}
        if (state.Workspace == Workspace.Sample) {
            AddSampleEditor();
            if (connection.UsbPlaybackCapabilities.Supported)
                Section("sample.usbreturn", "FM1 USB return", AddUsbPlaybackControls);
            return;
        }
        }
        finally { StyleContent(content); }
    }

    private void AddLevelControl()
    {
        var snapshot = connection.Snapshot;
        var track = state.SelectedTrack;
        if (snapshot.LevelDescriptor is not { } descriptor || track >= snapshot.Levels.Count)
        {
            content.AddView(Label(track == 3 ? "Drum mixer control will be added separately." :
                "Connect compatible firmware to edit this synth's level.", 14));
            return;
        }
        var value = snapshot.Levels[track];
        var title = Label($"Track level · {value} (device units)");
        content.AddView(title);
        var slider = new SeekBar(this) { Max = descriptor.Maximum - descriptor.Minimum,
            Progress = value - descriptor.Minimum, Enabled = !snapshot.Busy,
            ContentDescription = "Synth track level" };
        slider.ProgressChanged += (_, args) => title.Text = $"Track level · {args.Progress + descriptor.Minimum} (device units)";
        content.AddView(slider);
        var apply = new Button(this) { Text = "Apply level", Enabled = !snapshot.Busy };
        apply.Click += async (_, _) => await connection.SetLevelAsync(track, slider.Progress + descriptor.Minimum);
        content.AddView(apply);
        var refresh = new Button(this) { Text = "Read level from FM1", Enabled = !snapshot.Busy };
        refresh.Click += async (_, _) => await connection.RefreshLevelAsync(track);
        content.AddView(refresh);
        content.AddView(Label("Apply waits for the device's acknowledged value. Use Read after changing the hardware knob; live synchronization is coming next.", 14));
    }

    private sealed class SystemInsets(int spacing) : Java.Lang.Object, View.IOnApplyWindowInsetsListener
    {
        public WindowInsets OnApplyWindowInsets(View view, WindowInsets insets)
        {
            if (OperatingSystem.IsAndroidVersionAtLeast(30))
            {
                var bars = insets.GetInsets(WindowInsets.Type.SystemBars() | WindowInsets.Type.DisplayCutout());
                view.SetPadding(spacing + bars.Left, spacing + bars.Top, spacing + bars.Right, spacing + bars.Bottom);
            }
            else
                view.SetPadding(spacing + insets.SystemWindowInsetLeft, spacing + insets.SystemWindowInsetTop,
                    spacing + insets.SystemWindowInsetRight, spacing + insets.SystemWindowInsetBottom);
            return insets;
        }
    }
}
