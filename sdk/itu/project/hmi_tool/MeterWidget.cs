﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Globalization;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;
using System.Drawing.Design;
using System.Drawing.Imaging;

namespace GUIDesigner
{
    class MeterWidget : Panel, IWidget
    {
        public MeterWidget()
        {
            base.Margin = new Padding(0, 0, 0, 0);
            this.BackColor = Color.Transparent;
            this.Compress = true;
            this.External = false;
            this.Dither = true;
            this.StartAngle = 0;
            this.EndAngle = 360;
            this.MaxValue = 100;
            this.MinRadius = 0;
            this.SupportTouch = true;
            this.Delay = 0;
            this.Action01 = new WidgetAction(ITU.WidgetActionType.None, WidgetEvent.Changed, "", "");
            this.Action02 = new WidgetAction(ITU.WidgetActionType.None, WidgetEvent.Changed, "", "");
            this.Action03 = new WidgetAction(ITU.WidgetActionType.None, WidgetEvent.Changed, "", "");
            this.Action04 = new WidgetAction(ITU.WidgetActionType.None, WidgetEvent.Changed, "", "");
            this.Action05 = new WidgetAction(ITU.WidgetActionType.None, WidgetEvent.Changed, "", "");
            this.Action06 = new WidgetAction(ITU.WidgetActionType.None, WidgetEvent.Changed, "", "");
            this.Action07 = new WidgetAction(ITU.WidgetActionType.None, WidgetEvent.Changed, "", "");
            this.Action08 = new WidgetAction(ITU.WidgetActionType.None, WidgetEvent.Changed, "", "");
            this.Action09 = new WidgetAction(ITU.WidgetActionType.None, WidgetEvent.Changed, "", "");
            this.Action10 = new WidgetAction(ITU.WidgetActionType.None, WidgetEvent.Changed, "", "");
            this.Action11 = new WidgetAction(ITU.WidgetActionType.None, WidgetEvent.Changed, "", "");
            this.Action12 = new WidgetAction(ITU.WidgetActionType.None, WidgetEvent.Changed, "", "");
            this.Action13 = new WidgetAction(ITU.WidgetActionType.None, WidgetEvent.Changed, "", "");
            this.Action14 = new WidgetAction(ITU.WidgetActionType.None, WidgetEvent.Changed, "", "");
            this.Action15 = new WidgetAction(ITU.WidgetActionType.None, WidgetEvent.Changed, "", "");
        }

        [Editor(typeof(ImageLocationEditor), typeof(UITypeEditor))]
        public override Image BackgroundImage
        {
            get { return base.BackgroundImage; }
            set { base.BackgroundImage = value; }
        }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public virtual ImageLayout BackgroundImageLayout { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public BorderStyle BorderStyle { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public virtual Cursor Cursor { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public virtual Font Font { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public virtual Color ForeColor { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public virtual RightToLeft RightToLeft { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public bool UseWaitCursor { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public virtual bool AllowDrop { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public virtual ContextMenuStrip ContextMenuStrip { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public bool Enabled { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public ImeMode ImeMode { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public int TabIndex { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public bool TabStop { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public string AccessibleDescription { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public string AccessibleName { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public AccessibleRole AccessibleRole { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public virtual AnchorStyles Anchor { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public virtual bool AutoScroll { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public Size AutoScrollMargin { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public Size AutoScrollMinSize { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public override bool AutoSize { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public virtual AutoSizeMode AutoSizeMode { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public virtual DockStyle Dock { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public Padding Margin { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public virtual Size MaximumSize { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public virtual Size MinimumSize { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public Padding Padding { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public bool CausesValidation { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public ControlBindingsCollection DataBindings { get; set; }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public object Tag { get; set; }

        [LocalizedDescription("PixelFormat", typeof(WidgetManual))]
        public ITU.WidgetPixelFormat PixelFormat { get; set; }
        [LocalizedDescription("Compress", typeof(WidgetManual))]
        public Boolean Compress { get; set; }
        [LocalizedDescription("External", typeof(WidgetManual))]
        public Boolean External { get; set; }
        public Boolean Dither { get; set; }

        public class StringListConverter : TypeConverter
        {
            // Convert from a string.
            public override object ConvertFrom(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value)
            {
                if (value.GetType() == typeof(string))
                {
                    return value;
                }
                else
                {
                    return base.ConvertFrom(context, culture, value);
                }
            }

            public override bool
            GetStandardValuesSupported(ITypeDescriptorContext context)
            {
                return true; // display drop
            }
            public override bool
            GetStandardValuesExclusive(ITypeDescriptorContext context)
            {
                return true; // drop-down vs combo
            }
            public override StandardValuesCollection
            GetStandardValues(ITypeDescriptorContext context)
            {
                // note you can also look at context etc to build list
                return new StandardValuesCollection(NameCreationService.names.ToArray());
            }
        }

        [LocalizedDescription("Value", typeof(WidgetManual))]
        public int Value { get; set; }

        [LocalizedDescription("PointerTarget", typeof(WidgetManual))]
        [TypeConverter(typeof(StringListConverter))]
        public String PointerTarget { get; set; }
        [LocalizedDescription("PointerX", typeof(WidgetManual))]
        public int PointerX { get; set; }
        [LocalizedDescription("PointerY", typeof(WidgetManual))]
        public int PointerY { get; set; }

        [LocalizedDescription("StartAngle", typeof(WidgetManual))]
        public int StartAngle { get; set; }
        [LocalizedDescription("EndAngle", typeof(WidgetManual))]
        public int EndAngle { get; set; }
        [LocalizedDescription("MaxValue", typeof(WidgetManual))]
        public int MaxValue { get; set; }
        [LocalizedDescription("MinRadius", typeof(WidgetManual))]
        public int MinRadius { get; set; }

        [LocalizedDescription("SupportTouch", typeof(WidgetManual))]
        public Boolean SupportTouch { get; set; }

        [LocalizedDescription("Delay", typeof(WidgetManual))]
        public int Delay { get; set; }

        public enum WidgetEvent
        {
            Changed = 20
        };

        public class WidgetActionTypeConverter : TypeConverter
        {
            public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
            {
                if (sourceType == typeof(string)) return true;
                return base.CanConvertFrom(context, sourceType);
            }

            // Return true if we need to convert into a string.
            public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
            {
                if (destinationType == typeof(String)) return true;
                return base.CanConvertTo(context, destinationType);
            }

            // Convert from a string.
            public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
            {
                if (value.GetType() == typeof(string))
                {
                    // Split the string separated by commas.
                    string txt = (string)(value);
                    string[] fields = txt.Split(new char[] { ',' });

                    try
                    {
                        ITU.WidgetActionType a = (ITU.WidgetActionType)Enum.Parse(typeof(ITU.WidgetActionType), fields[0]);
                        WidgetEvent e = WidgetEvent.Changed;
                        try
                        {
                            e = (WidgetEvent)Enum.Parse(typeof(WidgetEvent), fields[1]);
                        }
                        catch
                        {
                            a = ITU.WidgetActionType.None;
                        }
                        return new WidgetAction(a, e, fields[2], fields[3]);
                    }
                    catch
                    {
                        throw new InvalidCastException(
                            "Cannot convert the string '" +
                            value.ToString() + "' into a Action");
                    }
                }
                else
                {
                    return base.ConvertFrom(context, culture, value);
                }
            }

            public override object ConvertTo(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value, Type destinationType)
            {
                if (destinationType == typeof(string)) return value.ToString();
                return base.ConvertTo(context, culture, value, destinationType);
            }

            public override bool GetPropertiesSupported(ITypeDescriptorContext context)
            {
                return true;
            }

            public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value, Attribute[] attributes)
            {
                return TypeDescriptor.GetProperties(value);
            }
        }

        public class WidgetStringListConverter : TypeConverter
        {
            public override bool
            GetStandardValuesSupported(ITypeDescriptorContext context)
            {
                return true; // display drop
            }
            //public override bool
            //GetStandardValuesExclusive(ITypeDescriptorContext context)
            //{
            //    return true; // drop-down vs combo
            //}
            public override StandardValuesCollection
            GetStandardValues(ITypeDescriptorContext context)
            {
                List<string> names = new List<string>();
                WidgetAction a = (WidgetAction)context.Instance;

                foreach (HostSurface hs in HostSurface.hostSurfaces)
                {
                    Uitlity.GetWidgetNamesByActionType(hs.formDesignerHost.Container.Components, a.Action, names);
                }
                names.Sort();
                return new StandardValuesCollection(names.ToArray());
            }

            public override bool CanConvertFrom(System.ComponentModel.ITypeDescriptorContext context, System.Type sourceType)
            {
                if (sourceType == typeof(string))
                    return true;
                else
                    return base.CanConvertFrom(context, sourceType);
            }

            public override object ConvertFrom(System.ComponentModel.ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value)
            {
                if (value.GetType() == typeof(string))
                {
                    string s = value as string;
                    //NameCreationService.AddName(s);
                    return s;
                }
                else
                    return base.ConvertFrom(context, culture, value);
            }
        }

        [Serializable]
        [TypeConverter(typeof(WidgetActionTypeConverter))]
        public struct WidgetAction
        {
            public ITU.WidgetActionType Action { get; set; }
            public WidgetEvent Event { get; set; }

            private volatile string target;
            [TypeConverter(typeof(WidgetStringListConverter))]
            public String Target
            {
                get
                {
                    return target;
                }

                set
                {
                    if (Action == ITU.WidgetActionType.Function && ITU.enterKeyPressed && ITU.layerWidget != null && ITU.projectPath != null)
                    {
                        string funcName = value.Trim();

                        CodeGenerator.InvokeVisualStudio(funcName);

                        target = funcName;
                    }
                    else
                    {
                        target = value;
                    }
                }
            }

            public String Parameter { get; set; }

            public override string ToString()
            {
                return Action + "," + Event + "," + Target + "," + Parameter;
            }

            public WidgetAction(ITU.WidgetActionType action, WidgetEvent ev, String target, String param)
                : this()
            {
                Event = ev;
                Action = action;
                Target = target;
                Parameter = param;
            }
        }

        [LocalizedDescription("Action01to15", typeof(WidgetManual))]
        public WidgetAction Action01 { get; set; }
        [LocalizedDescription("Action01to15", typeof(WidgetManual))]
        public WidgetAction Action02 { get; set; }
        [LocalizedDescription("Action01to15", typeof(WidgetManual))]
        public WidgetAction Action03 { get; set; }
        [LocalizedDescription("Action01to15", typeof(WidgetManual))]
        public WidgetAction Action04 { get; set; }
        [LocalizedDescription("Action01to15", typeof(WidgetManual))]
        public WidgetAction Action05 { get; set; }
        [LocalizedDescription("Action01to15", typeof(WidgetManual))]
        public WidgetAction Action06 { get; set; }
        [LocalizedDescription("Action01to15", typeof(WidgetManual))]
        public WidgetAction Action07 { get; set; }
        [LocalizedDescription("Action01to15", typeof(WidgetManual))]
        public WidgetAction Action08 { get; set; }
        [LocalizedDescription("Action01to15", typeof(WidgetManual))]
        public WidgetAction Action09 { get; set; }
        [LocalizedDescription("Action01to15", typeof(WidgetManual))]
        public WidgetAction Action10 { get; set; }
        [LocalizedDescription("Action01to15", typeof(WidgetManual))]
        public WidgetAction Action11 { get; set; }
        [LocalizedDescription("Action01to15", typeof(WidgetManual))]
        public WidgetAction Action12 { get; set; }
        [LocalizedDescription("Action01to15", typeof(WidgetManual))]
        public WidgetAction Action13 { get; set; }
        [LocalizedDescription("Action01to15", typeof(WidgetManual))]
        public WidgetAction Action14 { get; set; }
        [LocalizedDescription("Action01to15", typeof(WidgetManual))]
        public WidgetAction Action15 { get; set; }

        private bool hided = false;
        [LocalizedDescription("Hided", typeof(WidgetManual))]
        public bool Hided
        {
            get
            {
                return hided;
            }

            set
            {
                if (value)
                    Hide();
                else
                    Show();

                hided = value;
            }
        }

        private int shadowmode_c = 0;
        [LocalizedDescription("ShadowMode", typeof(WidgetManual))]
        public int ShadowMode
        {
            get { return shadowmode_c; }
            set
            {
                if (value < 0)
                {
                    shadowmode_c = 0;
                }
                else if (value > 3)
                {
                    shadowmode_c = 3;
                }
                else
                {
                    shadowmode_c = value;
                }
            }
        }

        private int shadow_loop_max_c = 4;
        [LocalizedDescription("ShadowLoopMax", typeof(WidgetManual))]
        public int ShadowLoopMax
        {
            get { return shadow_loop_max_c; }
            set
            {
                if (value < 1)
                {
                    shadow_loop_max_c = 1;
                }
                else if (value > 4)
                {
                    shadow_loop_max_c = 4;
                }
                else
                {
                    shadow_loop_max_c = value;
                }
            }
        }

        [LocalizedDescription("CW_Shadow1", typeof(WidgetManual))]
        [Editor(typeof(ImageLocationEditor), typeof(UITypeEditor))]
        [DefaultValue(typeof(Image), null)]
        public Image CW_Shadow1 { get; set; }

        [LocalizedDescription("CW_Shadow2", typeof(WidgetManual))]
        [Editor(typeof(ImageLocationEditor), typeof(UITypeEditor))]
        [DefaultValue(typeof(Image), null)]
        public Image CW_Shadow2 { get; set; }

        [LocalizedDescription("CW_Shadow3", typeof(WidgetManual))]
        [Editor(typeof(ImageLocationEditor), typeof(UITypeEditor))]
        [DefaultValue(typeof(Image), null)]
        public Image CW_Shadow3 { get; set; }

        [LocalizedDescription("ACW_Shadow1", typeof(WidgetManual))]
        [Editor(typeof(ImageLocationEditor), typeof(UITypeEditor))]
        [DefaultValue(typeof(Image), null)]
        public Image ACW_Shadow1 { get; set; }

        [LocalizedDescription("ACW_Shadow2", typeof(WidgetManual))]
        [Editor(typeof(ImageLocationEditor), typeof(UITypeEditor))]
        [DefaultValue(typeof(Image), null)]
        public Image ACW_Shadow2 { get; set; }

        [LocalizedDescription("ACW_Shadow3", typeof(WidgetManual))]
        [Editor(typeof(ImageLocationEditor), typeof(UITypeEditor))]
        [DefaultValue(typeof(Image), null)]
        public Image ACW_Shadow3 { get; set; }

        private bool hide_ds = false;
        public bool HideDS
        {
            get
            {
                return hide_ds;
            }
            set
            {
                hide_ds = value;
            }
        }

        public ITUWidget CreateITUWidget()
        {
            ITUMeter meter = new ITUMeter();

            meter.name = this.Name;

            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this);
            meter.visible = (bool)properties["Visible"].GetValue(this);

            meter.active = false;
            meter.dirty = false;
            meter.alpha = 255;
            meter.rect.x = this.Location.X;
            meter.rect.y = this.Location.Y;
            meter.rect.width = this.Size.Width;
            meter.rect.height = this.Size.Height;
            meter.color.alpha = this.BackColor.A;
            meter.color.red = this.BackColor.R;
            meter.color.green = this.BackColor.G;
            meter.color.blue = this.BackColor.B;
            meter.bound.x = 0;
            meter.bound.y = 0;
            meter.bound.width = 0;
            meter.bound.height = 0;

            if (this.BackgroundImage != null)
            {
                meter.staticSurf = ITU.CreateSurfaceNode(this.BackgroundImage as Bitmap, this.PixelFormat, this.Compress, this.External, this.Dither);
            }
            if (this.External)
                meter.flags |= ITU.ITU_EXTERNAL;

            meter.value = this.Value;
            meter.pointerName = this.PointerTarget;
            meter.pointerX = this.PointerX;
            meter.pointerY = this.PointerY;
            meter.startAngle = this.StartAngle;
            meter.endAngle = this.EndAngle;
            meter.maxValue = this.MaxValue;
            meter.minRadius = this.MinRadius;

            if (!this.SupportTouch)
                meter.flags &= ~ITU.ITU_ENABLED;

            meter.delay = this.Delay;

            meter.lastvalue = this.Value;
            meter.shadowmode = this.ShadowMode;
            meter.diffvalue = 0;
            meter.shadow_loop_count = 0;
            meter.shadow_loop_max = this.ShadowLoopMax;

            //image data
            if (this.CW_Shadow1 != null)
            {
                meter.cw_shadow[0] = ITU.CreateSurfaceNode(this.CW_Shadow1 as Bitmap, ITU.WidgetPixelFormat.ARGB8888, false, false, false);
            }
            if (this.CW_Shadow2 != null)
            {
                meter.cw_shadow[1] = ITU.CreateSurfaceNode(this.CW_Shadow2 as Bitmap, ITU.WidgetPixelFormat.ARGB8888, false, false, false);
            }
            if (this.CW_Shadow3 != null)
            {
                meter.cw_shadow[2] = ITU.CreateSurfaceNode(this.CW_Shadow3 as Bitmap, ITU.WidgetPixelFormat.ARGB8888, false, false, false);
            }
            if (this.ACW_Shadow1 != null)
            {
                meter.acw_shadow[0] = ITU.CreateSurfaceNode(this.ACW_Shadow1 as Bitmap, ITU.WidgetPixelFormat.ARGB8888, false, false, false);
            }
            if (this.ACW_Shadow2 != null)
            {
                meter.acw_shadow[1] = ITU.CreateSurfaceNode(this.ACW_Shadow2 as Bitmap, ITU.WidgetPixelFormat.ARGB8888, false, false, false);
            }
            if (this.ACW_Shadow3 != null)
            {
                meter.acw_shadow[2] = ITU.CreateSurfaceNode(this.ACW_Shadow3 as Bitmap, ITU.WidgetPixelFormat.ARGB8888, false, false, false);
            }

            meter.actions[0].action = (ITUActionType)this.Action01.Action;
            meter.actions[0].ev = (ITUEvent)this.Action01.Event;
            meter.actions[0].target = this.Action01.Target;
            meter.actions[0].param = this.Action01.Parameter;
            meter.actions[1].action = (ITUActionType)this.Action02.Action;
            meter.actions[1].ev = (ITUEvent)this.Action02.Event;
            meter.actions[1].target = this.Action02.Target;
            meter.actions[1].param = this.Action02.Parameter;
            meter.actions[2].action = (ITUActionType)this.Action03.Action;
            meter.actions[2].ev = (ITUEvent)this.Action03.Event;
            meter.actions[2].target = this.Action03.Target;
            meter.actions[2].param = this.Action03.Parameter;
            meter.actions[3].action = (ITUActionType)this.Action04.Action;
            meter.actions[3].ev = (ITUEvent)this.Action04.Event;
            meter.actions[3].target = this.Action04.Target;
            meter.actions[3].param = this.Action04.Parameter;
            meter.actions[4].action = (ITUActionType)this.Action05.Action;
            meter.actions[4].ev = (ITUEvent)this.Action05.Event;
            meter.actions[4].target = this.Action05.Target;
            meter.actions[4].param = this.Action05.Parameter;
            meter.actions[5].action = (ITUActionType)this.Action06.Action;
            meter.actions[5].ev = (ITUEvent)this.Action06.Event;
            meter.actions[5].target = this.Action06.Target;
            meter.actions[5].param = this.Action06.Parameter;
            meter.actions[6].action = (ITUActionType)this.Action07.Action;
            meter.actions[6].ev = (ITUEvent)this.Action07.Event;
            meter.actions[6].target = this.Action07.Target;
            meter.actions[6].param = this.Action07.Parameter;
            meter.actions[7].action = (ITUActionType)this.Action08.Action;
            meter.actions[7].ev = (ITUEvent)this.Action08.Event;
            meter.actions[7].target = this.Action08.Target;
            meter.actions[7].param = this.Action08.Parameter;
            meter.actions[8].action = (ITUActionType)this.Action09.Action;
            meter.actions[8].ev = (ITUEvent)this.Action09.Event;
            meter.actions[8].target = this.Action09.Target;
            meter.actions[8].param = this.Action09.Parameter;
            meter.actions[9].action = (ITUActionType)this.Action10.Action;
            meter.actions[9].ev = (ITUEvent)this.Action10.Event;
            meter.actions[9].target = this.Action10.Target;
            meter.actions[9].param = this.Action10.Parameter;
            meter.actions[10].action = (ITUActionType)this.Action11.Action;
            meter.actions[10].ev = (ITUEvent)this.Action11.Event;
            meter.actions[10].target = this.Action11.Target;
            meter.actions[10].param = this.Action11.Parameter;
            meter.actions[11].action = (ITUActionType)this.Action12.Action;
            meter.actions[11].ev = (ITUEvent)this.Action12.Event;
            meter.actions[11].target = this.Action12.Target;
            meter.actions[11].param = this.Action12.Parameter;
            meter.actions[12].action = (ITUActionType)this.Action13.Action;
            meter.actions[12].ev = (ITUEvent)this.Action13.Event;
            meter.actions[12].target = this.Action13.Target;
            meter.actions[12].param = this.Action13.Parameter;
            meter.actions[13].action = (ITUActionType)this.Action14.Action;
            meter.actions[13].ev = (ITUEvent)this.Action14.Event;
            meter.actions[13].target = this.Action14.Target;
            meter.actions[13].param = this.Action14.Parameter;
            meter.actions[14].action = (ITUActionType)this.Action15.Action;
            meter.actions[14].ev = (ITUEvent)this.Action15.Event;
            meter.actions[14].target = this.Action15.Target;
            meter.actions[14].param = this.Action15.Parameter;

            return meter;
        }

        public void SaveImages(String path)
        {
            if (this.BackgroundImage != null)
            {
                Bitmap bitmap = this.BackgroundImage as Bitmap;
                ITU.SaveImage(bitmap, path, LayerWidget.FindLayerName(this), this.Name + "_BackgroundImage");
            }
            if (this.CW_Shadow1 != null)
            {
                Bitmap bitmap = this.CW_Shadow1 as Bitmap;
                ITU.SaveImage(bitmap, path, LayerWidget.FindLayerName(this), this.Name + "_CW_Shadow1");
            }
            if (this.CW_Shadow2 != null)
            {
                Bitmap bitmap = this.CW_Shadow2 as Bitmap;
                ITU.SaveImage(bitmap, path, LayerWidget.FindLayerName(this), this.Name + "_CW_Shadow2");
            }
            if (this.CW_Shadow3 != null)
            {
                Bitmap bitmap = this.CW_Shadow3 as Bitmap;
                ITU.SaveImage(bitmap, path, LayerWidget.FindLayerName(this), this.Name + "_CW_Shadow3");
            }
            if (this.ACW_Shadow1 != null)
            {
                Bitmap bitmap = this.ACW_Shadow1 as Bitmap;
                ITU.SaveImage(bitmap, path, LayerWidget.FindLayerName(this), this.Name + "_ACW_Shadow1");
            }
            if (this.ACW_Shadow2 != null)
            {
                Bitmap bitmap = this.ACW_Shadow2 as Bitmap;
                ITU.SaveImage(bitmap, path, LayerWidget.FindLayerName(this), this.Name + "_ACW_Shadow2");
            }
            if (this.ACW_Shadow3 != null)
            {
                Bitmap bitmap = this.ACW_Shadow3 as Bitmap;
                ITU.SaveImage(bitmap, path, LayerWidget.FindLayerName(this), this.Name + "_ACW_Shadow3");
            }
        }

        public void WriteFunctions(HashSet<string> functions)
        {
            if (this.Action01.Action == ITU.WidgetActionType.Function)
                functions.Add(this.Action01.Target);
            if (this.Action02.Action == ITU.WidgetActionType.Function)
                functions.Add(this.Action02.Target);
            if (this.Action03.Action == ITU.WidgetActionType.Function)
                functions.Add(this.Action03.Target);
            if (this.Action04.Action == ITU.WidgetActionType.Function)
                functions.Add(this.Action04.Target);
            if (this.Action05.Action == ITU.WidgetActionType.Function)
                functions.Add(this.Action05.Target);
            if (this.Action06.Action == ITU.WidgetActionType.Function)
                functions.Add(this.Action06.Target);
            if (this.Action07.Action == ITU.WidgetActionType.Function)
                functions.Add(this.Action07.Target);
            if (this.Action08.Action == ITU.WidgetActionType.Function)
                functions.Add(this.Action08.Target);
            if (this.Action09.Action == ITU.WidgetActionType.Function)
                functions.Add(this.Action09.Target);
            if (this.Action10.Action == ITU.WidgetActionType.Function)
                functions.Add(this.Action10.Target);
            if (this.Action11.Action == ITU.WidgetActionType.Function)
                functions.Add(this.Action11.Target);
            if (this.Action12.Action == ITU.WidgetActionType.Function)
                functions.Add(this.Action12.Target);
            if (this.Action13.Action == ITU.WidgetActionType.Function)
                functions.Add(this.Action13.Target);
            if (this.Action14.Action == ITU.WidgetActionType.Function)
                functions.Add(this.Action14.Target);
            if (this.Action15.Action == ITU.WidgetActionType.Function)
                functions.Add(this.Action15.Target);
        }

        public bool HasFunctionName(string funcName)
        {
            if (this.Action01.Action == ITU.WidgetActionType.Function && this.Action01.Target == funcName)
                return true;
            if (this.Action02.Action == ITU.WidgetActionType.Function && this.Action02.Target == funcName)
                return true;
            if (this.Action03.Action == ITU.WidgetActionType.Function && this.Action03.Target == funcName)
                return true;
            if (this.Action04.Action == ITU.WidgetActionType.Function && this.Action04.Target == funcName)
                return true;
            if (this.Action05.Action == ITU.WidgetActionType.Function && this.Action05.Target == funcName)
                return true;
            if (this.Action06.Action == ITU.WidgetActionType.Function && this.Action06.Target == funcName)
                return true;
            if (this.Action07.Action == ITU.WidgetActionType.Function && this.Action07.Target == funcName)
                return true;
            if (this.Action08.Action == ITU.WidgetActionType.Function && this.Action08.Target == funcName)
                return true;
            if (this.Action09.Action == ITU.WidgetActionType.Function && this.Action09.Target == funcName)
                return true;
            if (this.Action10.Action == ITU.WidgetActionType.Function && this.Action10.Target == funcName)
                return true;
            if (this.Action11.Action == ITU.WidgetActionType.Function && this.Action11.Target == funcName)
                return true;
            if (this.Action12.Action == ITU.WidgetActionType.Function && this.Action12.Target == funcName)
                return true;
            if (this.Action13.Action == ITU.WidgetActionType.Function && this.Action13.Target == funcName)
                return true;
            if (this.Action14.Action == ITU.WidgetActionType.Function && this.Action14.Target == funcName)
                return true;
            if (this.Action15.Action == ITU.WidgetActionType.Function && this.Action15.Target == funcName)
                return true;
            return false;
        }
    }
}
