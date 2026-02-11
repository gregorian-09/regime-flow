/* Copyright (C) 2025 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

package TestJavaClient;

import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.GridBagConstraints;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;

import com.ib.client.Order;

public class AttachedOrdersDlg extends JDialog {
    private JTextField 	m_slOrderId = new JTextField();
    private JTextField 	m_slOrderType = new JTextField();
    private JTextField 	m_ptOrderId = new JTextField();
    private JTextField 	m_ptOrderType = new JTextField();

    Order m_order;

    private static final int COL1_WIDTH = 30 ;
    private static final int COL2_WIDTH = 100 - COL1_WIDTH ;

    AttachedOrdersDlg(Order order, JDialog owner) {
        super( owner, true);

        m_order = order;

        // create button panel
        JPanel buttonPanel = new JPanel();
        JButton btnOk = new JButton("OK");
        buttonPanel.add(btnOk);
        JButton btnReset = new JButton("Reset");
        buttonPanel.add(btnReset);
        JButton btnCancel = new JButton("Cancel");
        buttonPanel.add(btnCancel);

        // create action listeners
        btnOk.addActionListener(e -> onOk());
        btnReset.addActionListener(e -> onReset());
        btnCancel.addActionListener(e -> onCancel());

        GridBagConstraints gbc = new GridBagConstraints() ;
        gbc.fill = GridBagConstraints.BOTH ;
        gbc.anchor = GridBagConstraints.CENTER ;
        gbc.weighty = 100 ;
        gbc.fill = GridBagConstraints.BOTH ;
        gbc.gridheight = 1 ;

        // create mid panel
        IBGridBagPanel midPanel = new IBGridBagPanel();
        midPanel.setBorder(BorderFactory.createTitledBorder( "Attached Orders"));
        addGBComponent(midPanel, new JLabel( "Stop-Loss Order Id"), gbc, COL1_WIDTH, GridBagConstraints.RELATIVE) ;
        addGBComponent(midPanel, m_slOrderId, gbc, COL2_WIDTH, GridBagConstraints.REMAINDER) ;
        addGBComponent(midPanel, new JLabel( "Stop-Loss Order Type"), gbc, COL1_WIDTH, GridBagConstraints.RELATIVE) ;
        addGBComponent(midPanel, m_slOrderType, gbc, COL2_WIDTH, GridBagConstraints.REMAINDER) ;
        addGBComponent(midPanel, new JLabel( "Protit-Taker Order Id"), gbc, COL1_WIDTH, GridBagConstraints.RELATIVE) ;
        addGBComponent(midPanel, m_ptOrderId, gbc, COL2_WIDTH, GridBagConstraints.REMAINDER) ;
        addGBComponent(midPanel, new JLabel( "Profit-Taker Order Type"), gbc, COL1_WIDTH, GridBagConstraints.RELATIVE) ;
        addGBComponent(midPanel, m_ptOrderType, gbc, COL2_WIDTH, GridBagConstraints.REMAINDER) ;

        // create dlg box
        getContentPane().add( midPanel, BorderLayout.CENTER);
        getContentPane().add( buttonPanel, BorderLayout.SOUTH);
        setTitle("Attached Orders");
        pack();
    }

    private void onOk() {
        try {
            int slOrderId = !m_slOrderId.getText().isEmpty() ? Integer.parseInt(m_slOrderId.getText()) : Integer.MAX_VALUE;
            String slOrderType = m_slOrderType.getText().trim();
            int ptOrderId = !m_ptOrderId.getText().isEmpty() ? Integer.parseInt(m_ptOrderId.getText()) : Integer.MAX_VALUE;
            String ptOrderType = m_ptOrderType.getText().trim();

            m_order.slOrderId(slOrderId);
            m_order.slOrderType(slOrderType);
            m_order.ptOrderId(ptOrderId);
            m_order.ptOrderType(ptOrderType);

            setVisible(false);
        }
        catch ( Exception e) {
            Main.inform( this, "Error - " + e);
        }
    }

    private void onReset() {
        try {
            m_order.slOrderId(Integer.MAX_VALUE);
            m_order.slOrderType("");
            m_order.ptOrderId(Integer.MAX_VALUE);
            m_order.ptOrderType("");

            setVisible(false);
        }
        catch ( Exception e) {
            Main.inform( this, "Error - " + e);
        }
    }

    private void onCancel() {
        setVisible( false);
    }

    private static void addGBComponent(IBGridBagPanel panel, Component comp, GridBagConstraints gbc, int weightx, int gridwidth)
    {
        gbc.weightx = weightx;
        gbc.gridwidth = gridwidth;
        panel.setConstraints(comp, gbc);
        panel.add(comp, gbc);
    }
}
